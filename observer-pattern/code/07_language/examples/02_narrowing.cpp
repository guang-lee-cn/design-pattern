// Example 02: the receiver takes int, the notification carries double.
//
// Expected verdict: ACCEPTED by generations 2 and 3 (double narrows to int silently).
// Generation 1 depends on how it is written -- see the two variants below.
//
// This is the row that decides how much concepts actually buy. Nothing in
// std::invocable<F&, double> can reject this, because such a call IS valid.

#include "interfaces.hpp"

#if GEN == 1
struct BadReceiver : Observer {
  // With override: the compiler says the signature does not match.
  // Without override: it hides the pure virtual and the class stays abstract.
  // Both are compile errors, but they are DIFFERENT errors -- see run_language.sh.
  void OnReading(int value) override { (void)value; }
};
#else
struct BadReceiver {
  void operator()(int value) const { (void)value; }
};
#endif

int main() {
  Sensor sensor;
  BadReceiver receiver;
  ATTACH(sensor, receiver);
  return 0;
}
