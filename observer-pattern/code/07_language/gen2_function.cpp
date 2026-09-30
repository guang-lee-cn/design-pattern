// Generation 2 of 3: type-erased callables (std::vector<std::function<void(double)>>).
//
// Any callable works now -- a lambda, a free function, a member function via lambda. The
// interface is no longer a type you must inherit; it is a SHAPE.
//
// The price is identity: two std::function objects cannot be compared, so Detach can no
// longer take "the same thing you attached". It takes an index -- a handle the producer
// hands out. Compare with generation 1, where the pointer was the handle.
//
// Build: g++ -std=c++17 -Wall -Wextra gen2_function.cpp -o /tmp/g2 && /tmp/g2

#include <cstddef>
#include <cstdio>
#include <functional>
#include <utility>
#include <vector>

class Sensor {
 public:
  using Sink = std::function<void(double)>;

  void Attach(Sink sink) { sinks_.push_back(std::move(sink)); }

  // No Detach(Sink): std::function has no identity to compare against.
  void DetachAt(std::size_t index) { sinks_.erase(sinks_.begin() + index); }

  std::size_t size() const { return sinks_.size(); }

  void Tick(double value) {
    for (const Sink& sink : sinks_) {
      sink(value);
    }
  }

 private:
  std::vector<Sink> sinks_;
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
  Sensor sensor;
  ConsoleDisplay console;
  AuditDisplay audit;

  sensor.Attach([&console](double value) { console.Show(value); });
  sensor.Attach([&audit](double value) { audit.Show(value); });
  std::printf("-- receivers: %zu\n", sensor.size());

  sensor.Tick(25.0);
  sensor.Tick(26.5);

  sensor.DetachAt(1);  // by position, not by identity
  std::printf("-- receivers: %zu\n", sensor.size());

  sensor.Tick(28.0);
  return 0;
}
