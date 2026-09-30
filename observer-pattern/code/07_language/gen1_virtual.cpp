// Generation 1 of 3: an abstract base class (the 1994 shape).
//
// The interface IS a type. To receive notifications you must inherit from Observer, so a
// lambda or a free function cannot be a receiver without an adapter. What you get back is
// diagnosis: when the signature drifts, the compiler names YOUR class.
//
// Build: g++ -std=c++17 -Wall -Wextra gen1_virtual.cpp -o /tmp/g1 && /tmp/g1

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <vector>

class Observer {
 public:
  virtual ~Observer() = default;
  virtual void OnReading(double value) = 0;
};

class Sensor {
 public:
  void Attach(Observer* observer) { observers_.push_back(observer); }

  // Identity is the pointer, so Detach takes the same pointer that was attached.
  void Detach(Observer* observer) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                     observers_.end());
  }

  std::size_t size() const { return observers_.size(); }

  void Tick(double value) {
    for (Observer* observer : observers_) {
      observer->OnReading(value);
    }
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

int main() {
  Sensor sensor;
  ConsoleDisplay console;
  AuditDisplay audit;

  sensor.Attach(&console);
  sensor.Attach(&audit);
  std::printf("-- receivers: %zu\n", sensor.size());

  sensor.Tick(25.0);
  sensor.Tick(26.5);

  sensor.Detach(&audit);
  std::printf("-- receivers: %zu\n", sensor.size());

  sensor.Tick(28.0);
  return 0;
}
