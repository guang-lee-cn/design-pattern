// Generation 3 of 3: the same storage, with the shape written down as a constraint.
//
// Storage did not change -- still std::function<void(double)>. What changed is WHERE the
// check happens: generation 2 checks inside <functional> while converting the argument;
// generation 3 checks at the interface, and the constraint is part of what the caller reads.
//
// Read the interception matrix in docs/07-语言演进.md before assuming this catches more
// than generation 2. It does not: std::invocable<F&, double> is the same question
// std::function<void(double)> already asks. What it buys is the ERROR, not the check.
//
// Build: g++ -std=c++20 -Wall -Wextra gen3_concept.cpp -o /tmp/g3 && /tmp/g3

#include <concepts>
#include <cstddef>
#include <cstdio>
#include <functional>
#include <utility>
#include <vector>

class Sensor {
 public:
  using Sink = std::function<void(double)>;

  // Generation 2 spelled this "void Attach(Sink sink)". Same storage, but the argument no
  // longer converts silently: F is deduced and the requirement is stated in the signature.
  template <class F>
    requires std::invocable<F&, double>
  void Attach(F&& sink) {
    sinks_.emplace_back(std::forward<F>(sink));
  }

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

  sensor.DetachAt(1);
  std::printf("-- receivers: %zu\n", sensor.size());

  sensor.Tick(28.0);
  return 0;
}
