// Shared scenario for every form in this directory -- same events, same receivers,
// same print format, so the outputs can be compared byte for byte:
//
//   4 readings : 25.0 / 26.5 / 27.0 (normal) + 99.0 (out of range)
//   3 receivers: ConsoleDisplay, AuditDisplay, AlarmDisplay
//
// Form D of 5: one-shot callbacks -- "looks like a subscription, is not one".
//
// The registration call is indistinguishable from Attach(), and the first notification
// is byte-for-byte form A's. The difference shows up on the second one, and it is not a
// difference in degree: the list is empty. Nothing was unregistered -- it was never a
// subscription in the first place.
//
// Build: g++ -std=c++17 -Wall -Wextra form_d_one_shot.cpp -o /tmp/fd && /tmp/fd

#include <cstdio>
#include <functional>
#include <utility>
#include <vector>

class Sensor {
 public:
  using Handler = std::function<void(double)>;

  // The call site reads exactly like Attach(), and that is the whole problem.
  void RunOnce(Handler handler) { pending_.push_back(std::move(handler)); }

  void Tick(double value) {
    std::printf("-- tick %.1f\n", value);
    std::vector<Handler> batch;
    batch.swap(pending_);  // taken before the call, so a handler cannot re-arm into it
    for (const Handler& handler : batch) {
      handler(value);
    }
    std::printf("-- tick %.1f returned\n", value);
  }

 private:
  std::vector<Handler> pending_;
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

class AlarmDisplay {
 public:
  void Show(double value) const {
    std::printf(value > 90.0 ? "[alarm] OVER %.1f\n" : "[alarm] ok %.1f\n", value);
  }
};

int main() {
  Sensor sensor;
  ConsoleDisplay console;
  AuditDisplay audit;
  AlarmDisplay alarm;

  sensor.RunOnce([&](double value) { console.Show(value); });
  sensor.RunOnce([&](double value) { audit.Show(value); });
  sensor.RunOnce([&](double value) { alarm.Show(value); });

  const double kReadings[] = {25.0, 26.5, 27.0, 99.0};
  for (double value : kReadings) {
    sensor.Tick(value);
  }
  return 0;
}
