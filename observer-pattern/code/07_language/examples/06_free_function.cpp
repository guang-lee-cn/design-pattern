// Example 06: the receiver is a free function.
//
// Expected verdict: REJECTED by generation 1 (a function pointer is not an Observer), and
// ACCEPTED by generations 2 and 3 without an adapter. This is the expressiveness row: it is
// the reason the base class stopped being the default.
//
// Note that the offending line is identical for all three generations. Only the interface
// differs -- so this row measures the interface, not the call.

#include "interfaces.hpp"

void OnReadingFree(double value) { (void)value; }

int main() {
  Sensor sensor;
  sensor.Attach(&OnReadingFree);
  return 0;
}
