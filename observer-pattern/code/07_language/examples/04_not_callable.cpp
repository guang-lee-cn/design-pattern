// Example 04: the thing passed in is not callable at all.
//
// Expected verdict: rejected by all three generations.

#include "interfaces.hpp"

struct NotCallable {
  int marker = 0;
};

int main() {
  Sensor sensor;
  NotCallable thing;

#if GEN == 1
  // Not derived from Observer: no conversion exists.
  sensor.Attach(&thing);
#else
  sensor.Attach(thing);
#endif
  return 0;
}
