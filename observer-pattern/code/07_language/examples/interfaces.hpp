// The three interfaces, one per generation, selected by -DGEN=1|2|3.
//
// Only the receiving end is shown: the examples in this directory are about what the
// interface ACCEPTS, not about dispatching. Kept in one header so that each example is a
// two-line "offending call" rather than three copies of the same class.
//
//   GEN=1 -> g++ -std=c++17
//   GEN=2 -> g++ -std=c++17
//   GEN=3 -> g++ -std=c++20

#ifndef OBSERVER07_EXAMPLES_INTERFACES_HPP_
#define OBSERVER07_EXAMPLES_INTERFACES_HPP_

#if GEN == 1

#include <algorithm>
#include <cstddef>
#include <vector>

class Observer {
 public:
  virtual ~Observer() = default;
  virtual void OnReading(double value) = 0;
};

class Sensor {
 public:
  void Attach(Observer* observer) { observers_.push_back(observer); }

 private:
  std::vector<Observer*> observers_;
};

#define ATTACH(sensor, receiver) (sensor).Attach(&(receiver))

#elif GEN == 2

#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

class Sensor {
 public:
  using Sink = std::function<void(double)>;

  void Attach(Sink sink) { sinks_.push_back(std::move(sink)); }

 private:
  std::vector<Sink> sinks_;
};

#define ATTACH(sensor, receiver) (sensor).Attach((receiver))

#elif GEN == 3

#include <concepts>
#include <cstddef>
#include <functional>
#include <utility>
#include <vector>

class Sensor {
 public:
  using Sink = std::function<void(double)>;

  template <class F>
    requires std::invocable<F&, double>
  void Attach(F&& sink) {
    sinks_.emplace_back(std::forward<F>(sink));
  }

 private:
  std::vector<Sink> sinks_;
};

#define ATTACH(sensor, receiver) (sensor).Attach((receiver))

#else
#error "define GEN to 1, 2 or 3"
#endif

#endif  // OBSERVER07_EXAMPLES_INTERFACES_HPP_
