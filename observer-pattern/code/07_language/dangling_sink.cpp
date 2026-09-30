// Section 6 asked who reclaims the receiver list. This file asks the same question after the
// interface stopped being a type.
//
// Generation 1: the list holds Observer*, so the receiver has a NAME in the producer's source
//               (AuditDisplay) and a lifetime bug can be grepped for.
// Generation 2: the list holds std::function<void(double)>. The receiver's type is hidden
//               inside the erased object -- nothing in the producer's code names it. The bug
//               is identical and the search is harder.
//
// Build plain: g++ -std=c++17 -Wall -Wextra dangling_sink.cpp -o /tmp/d7 && /tmp/d7
// Build asan : g++ -std=c++17 -Wall -Wextra -fsanitize=address -g dangling_sink.cpp -o /tmp/d7a \
//              && /tmp/d7a

#include <cstddef>
#include <cstdio>
#include <functional>
#include <utility>
#include <vector>

class Sensor {
 public:
  using Sink = std::function<void(double)>;

  void Attach(Sink sink) { sinks_.push_back(std::move(sink)); }
  std::size_t size() const { return sinks_.size(); }

  void Tick(double value) {
    std::printf("-- publish %.1f (sinks=%zu)\n", value, sinks_.size());
    for (const Sink& sink : sinks_) {
      sink(value);
    }
    std::printf("-- publish %.1f returned\n", value);
  }

 private:
  std::vector<Sink> sinks_;
};

class AuditDisplay {
 public:
  void Show(double value) {
    total_ += value;
    std::printf("    [audit] got %.1f (total %.1f)\n", value, total_);
  }

 private:
  double total_ = 0.0;
};

int main() {
  Sensor sensor;
  {
    AuditDisplay audit;
    // What is captured is a reference to a stack object that this scope is about to end.
    // Nothing in the Sink type records that. Compare section 6, where the list at least
    // spelled the receiver type out.
    sensor.Attach([&audit](double value) { audit.Show(value); });
    sensor.Tick(25.0);
    std::printf("   (leaving the scope that owns the receiver)\n");
  }

  sensor.Tick(28.0);  // the sink still refers to a dead object
  return 0;
}
