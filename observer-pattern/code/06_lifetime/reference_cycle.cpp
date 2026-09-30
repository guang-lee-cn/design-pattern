// Lifetime debt, case 3 of 3: the cycle.
//
// Krasner & Pope named this one in 1988, before C++ had a standard library
// (JOOP 1(3), the "DependentFields dictionary" paragraph):
//
//   "Since views and controllers hold direct pointers to their models, the
//    DependentFields dictionary creates a type of circularity that most storage
//    managers cannot reclaim."
//
// In modern C++ the same circle is written as two shared_ptrs, and the symptom is not a
// crash: it is that NEITHER destructor ever runs. Two objects leak together, quietly.
//
// The circle exists because the two directions are produced by two different needs and
// get written in two different places:
//
//   Display -> Sensor   the "pull" side (the view asks the model for the current value)
//   Sensor  -> Display  the "push" side (the model keeps the list of dependents)
//
// Measurement: each class counts its own constructions and destructions. A program that
// dropped its last external reference should show "built 1, destroyed 1" on both sides.
//
// Build: g++ -std=c++17 -Wall -Wextra reference_cycle.cpp -o /tmp/d6c && /tmp/d6c
// Asan : g++ -std=c++17 -Wall -Wextra -fsanitize=address -g reference_cycle.cpp -o /tmp/d6ca && /tmp/d6ca

#include <cstdio>
#include <memory>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------------
// The cycle: both directions are shared_ptr
// ---------------------------------------------------------------------------------

class SensorShared;

class DisplayShared {
 public:
  DisplayShared(std::string name, std::shared_ptr<SensorShared> sensor)
      : name_(std::move(name)), sensor_(std::move(sensor)) {
    ++built_;
  }
  ~DisplayShared() { ++destroyed_; }

  void OnReading(double value) {
    std::printf("    [%s] got %.1f\n", name_.c_str(), value);
  }

  static int built() { return built_; }
  static int destroyed() { return destroyed_; }

 private:
  static int built_;
  static int destroyed_;
  std::string name_;
  std::shared_ptr<SensorShared> sensor_;  // Display -> Sensor (pull)
};

int DisplayShared::built_ = 0;
int DisplayShared::destroyed_ = 0;

class SensorShared {
 public:
  SensorShared() { ++built_; }
  ~SensorShared() { ++destroyed_; }

  void Attach(std::shared_ptr<DisplayShared> view) { views_.push_back(std::move(view)); }

  void Tick(double value) {
    std::printf("-- publish %.1f\n", value);
    for (const auto& view : views_) view->OnReading(value);
    std::printf("-- publish %.1f returned\n", value);
  }

  static int built() { return built_; }
  static int destroyed() { return destroyed_; }

 private:
  static int built_;
  static int destroyed_;
  std::vector<std::shared_ptr<DisplayShared>> views_;  // Sensor -> Display (push)
};

int SensorShared::built_ = 0;
int SensorShared::destroyed_ = 0;

// ---------------------------------------------------------------------------------
// The fix: the producer side holds a weak_ptr
// ---------------------------------------------------------------------------------

class SensorWeak;

class DisplayWeak {
 public:
  DisplayWeak(std::string name, std::shared_ptr<SensorWeak> sensor)
      : name_(std::move(name)), sensor_(std::move(sensor)) {
    ++built_;
  }
  ~DisplayWeak() { ++destroyed_; }

  void OnReading(double value) {
    std::printf("    [%s] got %.1f\n", name_.c_str(), value);
  }

  static int built() { return built_; }
  static int destroyed() { return destroyed_; }

 private:
  static int built_;
  static int destroyed_;
  std::string name_;
  std::shared_ptr<SensorWeak> sensor_;  // unchanged: still a strong pointer
};

int DisplayWeak::built_ = 0;
int DisplayWeak::destroyed_ = 0;

class SensorWeak {
 public:
  SensorWeak() { ++built_; }
  ~SensorWeak() { ++destroyed_; }

  void Attach(const std::shared_ptr<DisplayWeak>& view) { views_.push_back(view); }

  void Tick(double value) {
    std::printf("-- publish %.1f\n", value);
    // A weak handle says "if it is still there". This is also the natural place to drop
    // handles that have expired -- boost::signals2 calls the same thing
    // disconnect_expired_slot().
    std::size_t live = 0;
    for (const auto& weak : views_) {
      if (auto view = weak.lock()) {
        view->OnReading(value);
        ++live;
      }
    }
    std::printf("-- publish %.1f returned (live receivers: %zu of %zu)\n", value, live,
                views_.size());
  }

  static int built() { return built_; }
  static int destroyed() { return destroyed_; }

 private:
  static int built_;
  static int destroyed_;
  std::vector<std::weak_ptr<DisplayWeak>> views_;  // Sensor -> Display, non-owning
};

int SensorWeak::built_ = 0;
int SensorWeak::destroyed_ = 0;

namespace {

void Report(const char* label, int sensor_built, int sensor_gone, int display_built,
            int display_gone) {
  std::printf("   %s\n", label);
  std::printf("     Sensor : built %d, destroyed %d\n", sensor_built, sensor_gone);
  std::printf("     Display: built %d, destroyed %d\n", display_built, display_gone);
  std::printf("     => %s\n\n",
              (sensor_built == sensor_gone && display_built == display_gone)
                  ? "both objects were reclaimed"
                  : "the pair leaked together");
}

}  // namespace

int main() {
  std::printf("== both directions strong (the circle) ==\n");
  {
    auto sensor = std::make_shared<SensorShared>();
    auto display = std::make_shared<DisplayShared>("display-shared", sensor);
    sensor->Attach(display);
    sensor->Tick(20.0);

    // Drop both external references. Without a circle this is enough.
    sensor.reset();
    display.reset();
    std::printf("   (main dropped both of its references)\n");
  }
  Report("after leaving the scope:", SensorShared::built(), SensorShared::destroyed(),
         DisplayShared::built(), DisplayShared::destroyed());

  std::printf("== producer side downgraded to weak ==\n");
  {
    auto sensor = std::make_shared<SensorWeak>();
    auto display = std::make_shared<DisplayWeak>("display-weak", sensor);
    sensor->Attach(display);
    sensor->Tick(25.0);

    display.reset();
    std::printf("   (main dropped the display; the sensor is still alive)\n");
    sensor->Tick(26.0);  // the expired handle is simply skipped

    sensor.reset();
    std::printf("   (main dropped the sensor)\n");
  }
  Report("after leaving the scope:", SensorWeak::built(), SensorWeak::destroyed(),
         DisplayWeak::built(), DisplayWeak::destroyed());

  return 0;
}
