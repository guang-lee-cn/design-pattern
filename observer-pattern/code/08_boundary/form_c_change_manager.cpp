// Shared scenario for every form in this directory -- same events, same receivers,
// same print format, so the outputs can be compared byte for byte:
//
//   4 readings : 25.0 / 26.5 / 27.0 (normal) + 99.0 (out of range)
//   3 receivers: ConsoleDisplay, AuditDisplay, AlarmDisplay
//
// Form C of 5: ChangeManager -- GoF's own name for the case where the list moves out
// of the producer, and in the same book "an instance of the Mediator pattern".
//
//   Observer, Implementation: "When the dependency relationship between subjects and
//    observers is particularly complex, an object that maintains these relationships
//    might be required. We call such an object a ChangeManager."
//
//   ChangeManager has three responsibilities: "It maps a subject to its observers and
//    provides an interface to maintain this mapping. ... It defines a particular update
//    strategy. It updates all dependent observers at the request of a subject."
//
//   Related Patterns: "Mediator (273): By encapsulating complex update semantics, the
//    ChangeManager acts as mediator between subjects and observers."
//
// The output is deliberately byte-for-byte form A's. The list moved, the behaviour did
// not -- which is exactly why this form cannot be told apart by running it, and why the
// judgement has to be a structural one.
//
// Build: g++ -std=c++17 -Wall -Wextra form_c_change_manager.cpp -o /tmp/fc && /tmp/fc

#include <cstdio>
#include <vector>

class Sensor;

class Observer {
 public:
  virtual ~Observer() = default;
  virtual void OnReading(double value) = 0;
};

class ChangeManager {
 public:
  // Responsibility 1: map a subject to its observers.
  void Register(Sensor* sensor, Observer* observer) {
    subscriptions_.push_back(Subscription{sensor, observer});
  }

  void Unregister(Sensor* sensor, Observer* observer) {
    for (auto it = subscriptions_.begin(); it != subscriptions_.end(); ++it) {
      if (it->sensor == sensor && it->observer == observer) {
        subscriptions_.erase(it);
        return;
      }
    }
  }

  // Responsibilities 2 and 3: one update strategy, run at the subject's request.
  void Notify(Sensor* sensor, double value) {
    for (const Subscription& sub : subscriptions_) {
      if (sub.sensor == sensor) {
        sub.observer->OnReading(value);
      }
    }
  }

 private:
  struct Subscription {
    Sensor* sensor;
    Observer* observer;
  };

  std::vector<Subscription> subscriptions_;
};

class Sensor {
 public:
  explicit Sensor(ChangeManager& manager) : manager_(manager) {}

  void Tick(double value) {
    std::printf("-- tick %.1f\n", value);
    manager_.Notify(this, value);
    std::printf("-- tick %.1f returned\n", value);
  }

 private:
  ChangeManager& manager_;  // the producer's entire view of delivery
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
  ChangeManager manager;
  Sensor sensor(manager);
  ConsoleDisplay console;
  AuditDisplay audit;
  AlarmDisplay alarm;

  manager.Register(&sensor, &console);
  manager.Register(&sensor, &audit);
  manager.Register(&sensor, &alarm);

  const double kReadings[] = {25.0, 26.5, 27.0, 99.0};
  for (double value : kReadings) {
    sensor.Tick(value);
  }
  return 0;
}
