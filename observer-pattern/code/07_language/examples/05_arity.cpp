// Example 05: the receiver needs two arguments, the notification supplies one.
//
// Expected verdict: rejected by all three generations.

#include "interfaces.hpp"

#if GEN == 1
struct BadReceiver : Observer {
  void OnReading(double value, int extra) override {
    (void)value;
    (void)extra;
  }
};
#else
struct BadReceiver {
  void operator()(double value, int extra) const {
    (void)value;
    (void)extra;
  }
};
#endif

int main() {
  Sensor sensor;
  BadReceiver receiver;
  ATTACH(sensor, receiver);
  return 0;
}
