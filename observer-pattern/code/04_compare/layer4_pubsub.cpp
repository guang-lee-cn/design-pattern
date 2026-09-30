// Layer 4 of 4: publish-subscribe.
//
// Decoupling score 3/3 -- same mediator as layer 3, plus a queue and a dispatcher
// thread. Publish appends and returns, so the producer is decoupled from the receivers
// in space AND in time.
//
// The price is now visible instead of argued: after three publishes the producer has
// returned three times and no display has run yet.
//
// Start() is split out on purpose, so that "queued but not dispatched" can be counted
// deterministically. In a real system it is io_context::run(); calling it twice, as
// main() does below, is the same thing asio does after more work is posted.
//
// Build: g++ -std=c++17 -Wall -Wextra -pthread layer4_pubsub.cpp -o /tmp/l4 && /tmp/l4

#include <condition_variable>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

class EventBus {
 public:
  using Handler = std::function<void(double)>;

  EventBus() = default;
  ~EventBus() { Stop(); }

  EventBus(const EventBus&) = delete;
  EventBus& operator=(const EventBus&) = delete;

  int Subscribe(std::string_view topic, Handler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    const int id = next_id_++;
    subscriptions_.push_back(Subscription{std::string(topic), id, std::move(handler)});
    return id;
  }

  void Unsubscribe(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = subscriptions_.begin(); it != subscriptions_.end(); ++it) {
      if (it->id == id) {
        subscriptions_.erase(it);
        return;
      }
    }
  }

  // Asynchronous: this only appends. It returns before any handler has run.
  void Publish(std::string_view topic, double value) {
    {
      std::lock_guard<std::mutex> lock(mutex_);
      queue_.push(Event{std::string(topic), value});
    }
    ready_.notify_one();
  }

  std::size_t pending() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
  }

  std::size_t dispatched() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return dispatched_;
  }

  void Start() {
    if (worker_.joinable()) return;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      stopping_ = false;
    }
    worker_ = std::thread(&EventBus::DispatchLoop, this);
  }

  // Drain whatever is queued, then join. Safe to call more than once.
  void Stop() {
    if (!worker_.joinable()) return;
    {
      std::lock_guard<std::mutex> lock(mutex_);
      stopping_ = true;
    }
    ready_.notify_all();
    worker_.join();
  }

 private:
  struct Event {
    std::string topic;
    double value;
  };

  struct Subscription {
    std::string topic;
    int id;
    Handler handler;
  };

  void DispatchLoop() {
    for (;;) {
      double value = 0.0;
      std::vector<Handler> targets;
      {
        std::unique_lock<std::mutex> lock(mutex_);
        ready_.wait(lock, [this] { return !queue_.empty() || stopping_; });
        if (queue_.empty()) break;  // stopping_ set, and nothing left to drain
        value = queue_.front().value;
        const std::string topic = queue_.front().topic;
        queue_.pop();
        for (const Subscription& sub : subscriptions_) {
          if (sub.topic == topic) targets.push_back(sub.handler);
        }
      }
      // Handlers run outside the lock, so a handler may Subscribe or Unsubscribe.
      for (const Handler& handler : targets) handler(value);
      {
        std::lock_guard<std::mutex> lock(mutex_);
        ++dispatched_;
      }
    }
  }

  mutable std::mutex mutex_;
  std::condition_variable ready_;
  std::queue<Event> queue_;
  std::vector<Subscription> subscriptions_;
  std::thread worker_;
  int next_id_ = 1;
  bool stopping_ = false;
  std::size_t dispatched_ = 0;
};

class Sensor {
 public:
  Sensor(EventBus& bus, std::string topic) : bus_(bus), topic_(std::move(topic)) {}

  void Tick(double value) {
    std::printf("-- publish %.1f (dispatched so far=%zu)\n", value, bus_.dispatched());
    bus_.Publish(topic_, value);
    std::printf("-- publish %.1f returned (dispatched so far=%zu)\n", value,
                bus_.dispatched());
  }

 private:
  EventBus& bus_;
  std::string topic_;
};

class ConsoleDisplay {
 public:
  void Show(double value) const { std::printf("[console] got %.1f\n", value); }
};

class AuditDisplay {
 public:
  void Show(double value) {
    total_ += value;
    std::printf("[audit] got %.1f (total %.1f)\n", value, total_);
  }

 private:
  double total_ = 0.0;
};

int main() {
  EventBus bus;
  ConsoleDisplay console;
  AuditDisplay audit;

  bus.Subscribe("temperature", [&](double v) { console.Show(v); });
  const int audit_id = bus.Subscribe("temperature", [&](double v) { audit.Show(v); });

  Sensor sensor(bus, "temperature");
  sensor.Tick(25.0);
  sensor.Tick(26.5);
  sensor.Tick(27.0);
  std::printf("-- 3 publishes done: pending=%zu dispatched=%zu\n", bus.pending(),
              bus.dispatched());

  bus.Start();  // in a real system: io_context.run()
  bus.Stop();   // drain, then join
  std::printf("-- after drain: pending=%zu dispatched=%zu\n", bus.pending(),
              bus.dispatched());

  // --- cancel demo: the same three lines appear in all four files ---
  // It is equivalent to layer 3 here only because the queue was drained first. Had the
  // queue still held events, "unsubscribe" would have taken effect before them.
  std::printf("-- unsubscribe audit\n");
  bus.Unsubscribe(audit_id);
  sensor.Tick(28.0);
  bus.Start();  // run() again, asio-style
  bus.Stop();
  return 0;
}
