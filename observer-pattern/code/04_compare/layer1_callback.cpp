// Layer 1 of 4: callback.
//
// Decoupling score 0/3 -- the producer still knows its receiver in space, and still
// calls it in time. What it holds is ONE callable, not a list, so the receiver list
// (if there is one) lives at the CALL SITE: in main's lambda captures.
//
// Build: g++ -std=c++17 -Wall -Wextra layer1_callback.cpp -o /tmp/l1 && /tmp/l1

#include <cstdio>
#include <functional>
#include <utility>

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

class Sensor {
 public:
  using Sink = std::function<void(double)>;

  explicit Sensor(Sink sink) : sink_(std::move(sink)) {}

  void Tick(double value) {
    std::printf("-- publish %.1f\n", value);
    sink_(value);
    std::printf("-- publish %.1f returned\n", value);
  }
  // No Attach, no Detach, no size(): there is nothing here to count.

 private:
  Sink sink_;
};

int main() {
  ConsoleDisplay console;
  AuditDisplay audit;

  // The only receiver list in this program is right here, in the caller's captures.
  Sensor sensor([&](double value) {
    console.Show(value);
    audit.Show(value);
  });

  sensor.Tick(25.0);
  sensor.Tick(26.5);
  sensor.Tick(27.0);

  // --- cancel demo: the same three lines appear in all four files ---
  // Nothing can be written on this line. There is no handle to the audit display:
  // the only way to stop it is to replace the whole callable, and that stops the
  // console display too. Watch the output below -- both still receive 28.0.
  std::printf("-- unsubscribe audit\n");
  sensor.Tick(28.0);
  return 0;
}
