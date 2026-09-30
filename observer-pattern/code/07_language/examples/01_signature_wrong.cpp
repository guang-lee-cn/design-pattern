// Example 01: the receiver cannot accept a double at all -- it wants a std::string.
//
// Expected verdict: rejected by all three generations.
// The interesting part is not whether it fails, but WHERE the first error is reported.

#include <string>

#include "interfaces.hpp"

#if GEN == 1
struct BadReceiver : Observer {
  // Hides the pure virtual instead of overriding it, so the class stays abstract.
  void OnReading(std::string value) { (void)value; }
};
#else
struct BadReceiver {
  void operator()(std::string value) const { (void)value; }
};
#endif

int main() {
  Sensor sensor;
  BadReceiver receiver;
  ATTACH(sensor, receiver);
  return 0;
}
