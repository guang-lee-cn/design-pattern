// Layer 2 of 4: observer.
//
// Decoupling score 1/3 -- the receiver list moved INSIDE the producer, so the producer
// no longer knows which concrete types listen. It still holds a pointer to each
// receiver (space), and it still calls them inline (time).
//
// The new capability is the one layer 1 could not have: the producer can be asked how
// many listeners there are, and any one listener can be removed by name.
//
// The new cost is the contract. A second kind of event would force Observer to change,
// and therefore every receiver to change -- see check_growth.py, which gets the
// compiler to name the victims.
//
// Build: g++ -std=c++17 -Wall -Wextra layer2_observer.cpp -o /tmp/l2 && /tmp/l2

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

  void Detach(Observer* observer) {
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer),
                     observers_.end());
  }

  // Layer 1 had no equivalent of this line.
  std::size_t observer_count() const { return observers_.size(); }

  void Tick(double value) {
    std::printf("-- publish %.1f\n", value);
    for (Observer* observer : observers_) {
      observer->OnReading(value);
    }
    std::printf("-- publish %.1f returned\n", value);
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
  std::printf("producer-side receiver count: %zu\n", sensor.observer_count());

  sensor.Tick(25.0);
  sensor.Tick(26.5);
  sensor.Tick(27.0);

  // --- cancel demo: the same three lines appear in all four files ---
  std::printf("-- unsubscribe audit\n");
  sensor.Detach(&audit);
  sensor.Tick(28.0);
  std::printf("producer-side receiver count after detach: %zu\n",
              sensor.observer_count());
  return 0;
}
