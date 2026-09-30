// Lifetime debt, case 1 of 3: the receiver dies first.
//
// The producer holds a raw pointer, so "the receiver is gone" is not something it can
// ever find out. Two variants of the same mistake, and only one of them is detectable:
//
//   variant A  the address comes back, occupied by another receiver
//              -> the notification is delivered to an object that never subscribed, and
//                 no tool reports anything: as far as the process is concerned this is
//                 a perfectly healthy allocation.
//   variant B  the address stays free
//              -> without a sanitizer this is undefined behaviour (garbage or a crash);
//                 with -fsanitize=address it is a deterministic heap-use-after-free.
//
// Build plain: g++ -std=c++17 -Wall -Wextra dangling.cpp -o /tmp/d6 && /tmp/d6
// Build asan : g++ -std=c++17 -Wall -Wextra -fsanitize=address -g dangling.cpp -o /tmp/d6a && /tmp/d6a

#include <cstddef>
#include <cstdio>
#include <new>
#include <vector>

class Display {
 public:
  explicit Display(const char* name) : received_(0) {
    std::snprintf(name_, sizeof(name_), "%s", name);
  }

  void Show(double value) {
    ++received_;
    std::printf("    [%s @%p] got %.1f  (this object has now received %d)\n", name_,
                static_cast<const void*>(this), value, received_);
  }

  const char* name() const { return name_; }

 private:
  char name_[24];
  int received_;
};

class Sensor {
 public:
  // No destructor, no weak handle: the producer has no way to learn about a death.
  void Attach(Display* d) {
    viewers_.push_back(d);
    std::printf("    Attach(%s)\n", d->name());
  }

  void Tick(double value) {
    std::printf("-- publish %.1f  (producer's list holds %zu)\n", value, viewers_.size());
    for (Display* d : viewers_) {
      d->Show(value);
    }
    std::printf("-- publish %.1f returned\n", value);
  }

 private:
  std::vector<Display*> viewers_;
};

// The address is reused on purpose, so the experiment does not depend on what the
// compiler happens to do with stack slots.
static void VariantA() {
  std::printf("== variant A: the freed address is reused by a receiver that never "
              "subscribed ==\n");
  Sensor sensor;

  void* raw = ::operator new(sizeof(Display));
  Display* a = new (raw) Display("display-A");
  sensor.Attach(a);
  sensor.Tick(20.0);

  a->~Display();
  std::printf("   (display-A destroyed -- the producer was never told)\n");

  Display* b = new (raw) Display("display-B");
  std::printf("   (display-B constructed at the same address; it never called Attach)\n");
  sensor.Tick(25.0);

  b->~Display();
  ::operator delete(raw);
}

static void VariantB() {
  std::printf("\n== variant B: the freed address stays free ==\n");
  Sensor sensor;

  Display* c = new Display("display-C");
  sensor.Attach(c);
  sensor.Tick(30.0);

  delete c;
  std::printf("   (display-C deleted -- the producer was never told)\n");
  sensor.Tick(31.0);  // dangling read
}

int main() {
  VariantA();
  VariantB();
  std::printf("\nBoth variants left the producer's list non-empty; neither printed an "
              "error on its own.\n");
  return 0;
}
