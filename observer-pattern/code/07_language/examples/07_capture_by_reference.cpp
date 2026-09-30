// Example 07: a perfectly valid receiver that captures a local by reference.
//
// Expected verdict: ACCEPTED by all three generations. That is the point -- the section-6
// debt is not a type error, so no amount of interface refinement will catch it here.
//
// Caveat on symmetry: generation 1 cannot write a lambda, so its variant turns the reference
// into a member. The comparison is therefore between "a reference held by an erased callable"
// and "a reference held by an object" -- both invisible to the compiler.

#include "interfaces.hpp"

class ConsoleHooks {
 public:
  void Show(double value) const { (void)value; }
};

#if GEN == 1
class Forwarder : public Observer {
 public:
  explicit Forwarder(const ConsoleHooks& hooks) : hooks_(&hooks) {}
  void OnReading(double value) override { hooks_->Show(value); }

 private:
  const ConsoleHooks* hooks_;
};
#endif

int main() {
  Sensor sensor;
  ConsoleHooks hooks;

#if GEN == 1
  Forwarder forwarder(hooks);
  sensor.Attach(&forwarder);
#else
  sensor.Attach([&hooks](double value) { hooks.Show(value); });
#endif
  return 0;
}
