// Shared scenario for every form in this directory -- same events, same receivers,
// same print format, so the outputs can be compared byte for byte:
//
//   4 readings : 25.0 / 26.5 / 27.0 (normal) + 99.0 (out of range)
//   3 receivers: ConsoleDisplay, AuditDisplay, AlarmDisplay
//
// Form E of 5: chain of responsibility -- the notification has an answer.
//
// Same shape as form A with one difference: OnReading returns bool and a true return
// stops the walk. For three of the four readings nothing changes, so the first three
// blocks are byte-for-byte form A's. The difference exists only for a reading somebody
// objects to -- and the receiver that pays for it is the one BEHIND the objector:
// AlarmDisplay never hears about the 99.0.
//
// Real anchor: include/linux/notifier.h defines
//
//   #define NOTIFY_STOP_MASK 0x8000 /* Don't call further */
//
// so in the kernel a subscriber can end the chain for everybody behind it. That is the
// line where a "notification chain" stops being a notification and becomes a chain.
//
// Build: g++ -std=c++17 -Wall -Wextra form_e_chain.cpp -o /tmp/fe && /tmp/fe

#include <algorithm>
#include <cstdio>
#include <vector>

class Observer {
 public:
  virtual ~Observer() = default;
  virtual bool OnReading(double value) = 0;  // the answer is read by the producer
};

class Sensor {
 public:
  void Attach(Observer* observer) { observers_.push_back(observer); }

  void Detach(Observer* observer) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                     observers_.end());
  }

  void Tick(double value) {
    std::printf("-- tick %.1f\n", value);
    for (Observer* observer : observers_) {
      if (observer->OnReading(value)) {
        std::printf("-- chain stopped\n");
        break;
      }
    }
    std::printf("-- tick %.1f returned\n", value);
  }

 private:
  std::vector<Observer*> observers_;
};

class ConsoleDisplay : public Observer {
 public:
  bool OnReading(double value) override {
    std::printf("[console] got %.1f\n", value);
    return false;
  }
};

class AuditDisplay : public Observer {
 public:
  bool OnReading(double value) override {
    if (value > 90.0) {
      std::printf("[audit] rejects %.1f, stopping the chain\n", value);
      return true;  // equivalent to returning NOTIFY_STOP_MASK
    }
    total_ += value;
    std::printf("[audit] got %.1f (total %.1f)\n", value, total_);
    return false;
  }

 private:
  double total_ = 0.0;
};

class AlarmDisplay : public Observer {
 public:
  bool OnReading(double value) override {
    std::printf(value > 90.0 ? "[alarm] OVER %.1f\n" : "[alarm] ok %.1f\n", value);
    return false;
  }
};

int main() {
  Sensor sensor;
  ConsoleDisplay console;
  AuditDisplay audit;
  AlarmDisplay alarm;

  sensor.Attach(&console);
  sensor.Attach(&audit);
  sensor.Attach(&alarm);

  const double kReadings[] = {25.0, 26.5, 27.0, 99.0};
  for (double value : kReadings) {
    sensor.Tick(value);
  }
  return 0;
}
