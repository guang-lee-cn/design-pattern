// Shared scenario for every form in this directory -- same events, same receivers,
// same print format, so the outputs can be compared byte for byte:
//
//   4 readings : 25.0 / 26.5 / 27.0 (normal) + 99.0 (out of range)
//   3 receivers: ConsoleDisplay, AuditDisplay, AlarmDisplay
//
// Form B of 5: observer, pull model. GoF 1994 named both extremes:
//
//   "At one extreme, which we call the push model, the subject sends observers
//    detailed information about the change, whether they want it or not. At the
//    other extreme is the pull model; the subject sends nothing but the most
//    minimal notification, and observers ask for details explicitly thereafter.
//    The pull model emphasizes the subject's ignorance of its observers, whereas
//    the push model assumes subjects know something about their observers' needs."
//
// Identical to form A except for one line: OnReading takes the subject, not the value.
// That single line is what judgement Q4 is about -- it is visible in the signature, and
// it changes what a receiver is able to ask for.
//
// Build: g++ -std=c++17 -Wall -Wextra form_b_observer_pull.cpp -o /tmp/fb && /tmp/fb

#include <algorithm>
#include <cstdio>
#include <vector>

class Sensor;

class Observer {
 public:
  virtual ~Observer() = default;
  virtual void OnReading(const Sensor& subject) = 0;  // GoF: Update(Subject*)
};

class Sensor {
 public:
  void Attach(Observer* observer) { observers_.push_back(observer); }

  void Detach(Observer* observer) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                     observers_.end());
  }

  // What a receiver can ask for. Push never sees either of these.
  double value() const { return value_; }
  int sequence() const { return sequence_; }

  void Tick(double value) {
    value_ = value;
    ++sequence_;
    std::printf("-- tick %.1f\n", value);
    for (Observer* observer : observers_) {
      observer->OnReading(*this);
    }
    std::printf("-- tick %.1f returned\n", value);
  }

 private:
  std::vector<Observer*> observers_;
  double value_ = 0.0;
  int sequence_ = 0;
};

class ConsoleDisplay : public Observer {
 public:
  void OnReading(const Sensor& subject) override {
    std::printf("[console] got %.1f\n", subject.value());
  }
};

class AuditDisplay : public Observer {
 public:
  void OnReading(const Sensor& subject) override {
    total_ += subject.value();
    // The extra question push cannot answer.
    std::printf("[audit] reading #%d got %.1f (total %.1f)\n", subject.sequence(),
                subject.value(), total_);
  }

 private:
  double total_ = 0.0;
};

class AlarmDisplay : public Observer {
 public:
  void OnReading(const Sensor& subject) override {
    std::printf(subject.value() > 90.0 ? "[alarm] OVER %.1f\n" : "[alarm] ok %.1f\n",
                subject.value());
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
