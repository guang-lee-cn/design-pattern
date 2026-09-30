// Shared scenario for every form in this directory -- same events, same receivers,
// same print format, so the outputs can be compared byte for byte:
//
//   4 readings : 25.0 / 26.5 / 27.0 (normal) + 99.0 (out of range)
//   3 receivers: ConsoleDisplay, AuditDisplay, AlarmDisplay
//
// Form A of 5: observer, push model. This is the reference form.
//
// The receiver list lives inside the producer (vector<Observer*>), the producer
// hands each receiver the data itself, and the notification carries no answer:
// every receiver runs, every time.
//
// Build: g++ -std=c++17 -Wall -Wextra form_a_observer_push.cpp -o /tmp/fa && /tmp/fa

#include <algorithm>
#include <cstdio>
#include <vector>

class Observer {
 public:
  virtual ~Observer() = default;
  virtual void OnReading(double value) = 0;  // the data travels with the call
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
      observer->OnReading(value);
    }
    std::printf("-- tick %.1f returned\n", value);
  }

 private:
  std::vector<Observer*> observers_;
};

class ConsoleDisplay : public Observer {
 public:
  void OnReading(double value) override { std::printf("[console] got %.1f\n", value); }
};

class AuditDisplay : public Observer {
 public:
  void OnReading(double value) override {
    total_ += value;
    std::printf("[audit] got %.1f (total %.1f)\n", value, total_);
  }

 private:
  double total_ = 0.0;
};

class AlarmDisplay : public Observer {
 public:
  void OnReading(double value) override {
    std::printf(value > 90.0 ? "[alarm] OVER %.1f\n" : "[alarm] ok %.1f\n", value);
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
