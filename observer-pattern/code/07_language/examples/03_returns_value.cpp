// Example 03: the receiver returns a value, the notification returns nothing.
//
// Expected verdict: ACCEPTED by generations 2 and 3 -- the result is simply discarded.
// Generation 1 rejects it, but only because of override/abstract, not because anyone
// decided that return values are meaningful here.
//
// Cross-reference: section 5 read the Linux notifier chain, where the return value of a
// callback DOES stop the chain (NOTIFY_STOP_MASK). Same shape, opposite authority.

#include "interfaces.hpp"

#if GEN == 1
struct BadReceiver : Observer {
  double OnReading(double value) override { return value; }
};
#else
struct BadReceiver {
  double operator()(double value) const { return value; }
};
#endif

int main() {
  Sensor sensor;
  BadReceiver receiver;
  ATTACH(sensor, receiver);
  return 0;
}
