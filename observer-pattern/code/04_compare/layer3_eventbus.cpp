// Layer 3 of 4: in-process event bus.
//
// Decoupling score 2/3 -- the receiver list moved into a mediator, so the producer
// holds no reference to anybody: only a bus and a topic name. Space is decoupled.
// Time is NOT: Publish still returns only after every handler has run.
//
// Keep this file next to layer4: the two buses have the SAME interface. The only
// difference is what Publish does -- call now, or append for someone else to call.
//
// Build: g++ -std=c++17 -Wall -Wextra layer3_eventbus.cpp -o /tmp/l3 && /tmp/l3

#include <cstddef>
#include <cstdio>
#include <functional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class EventBus {
 public:
  using Handler = std::function<void(double)>;

  int Subscribe(std::string_view topic, Handler handler) {
    const int id = next_id_++;
    subscriptions_.push_back(Subscription{std::string(topic), id, std::move(handler)});
    return id;
  }

  void Unsubscribe(int id) {
    for (auto it = subscriptions_.begin(); it != subscriptions_.end(); ++it) {
      if (it->id == id) {
        subscriptions_.erase(it);
        return;
      }
    }
  }

  // Synchronous: when this returns, every matching handler has already run.
  void Publish(std::string_view topic, double value) {
    std::vector<Handler> targets;
    for (const Subscription& sub : subscriptions_) {
      if (sub.topic == topic) targets.push_back(sub.handler);
    }
    for (const Handler& handler : targets) handler(value);
  }

 private:
  struct Subscription {
    std::string topic;
    int id;
    Handler handler;
  };

  std::vector<Subscription> subscriptions_;
  int next_id_ = 1;
};

class Sensor {
 public:
  Sensor(EventBus& bus, std::string topic) : bus_(bus), topic_(std::move(topic)) {}

  void Tick(double value) {
    std::printf("-- publish %.1f\n", value);
    bus_.Publish(topic_, value);
    std::printf("-- publish %.1f returned\n", value);
  }

 private:
  EventBus& bus_;      // the whole of what the producer knows about delivery
  std::string topic_;  // ... and a name. No receiver list, no count.
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

  // --- cancel demo: the same three lines appear in all four files ---
  std::printf("-- unsubscribe audit\n");
  bus.Unsubscribe(audit_id);
  sensor.Tick(28.0);
  return 0;
}
