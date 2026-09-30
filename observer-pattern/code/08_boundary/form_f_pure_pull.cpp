// Shared scenario for every form in this directory -- same events, same receivers,
// same print format, so the outputs can be compared byte for byte:
//
//   4 readings : 25.0 / 26.5 / 27.0 (normal) + 99.0 (out of range)
//   3 receivers: ConsoleDisplay, AuditDisplay, AlarmDisplay
//
// Form F of 6: no notification at all -- the consumer pulls on its own schedule.
//
// There is no Attach, no list, and no call out of the producer: the sensor does not know
// anybody is watching it and has no way to find out. This is where judgement Q1 runs out
// -- not "the list lives somewhere else", but "there is no list".
//
// GoF puts the pull model at the other extreme of this axis, but the pull model still has
// a notification: "the subject sends nothing but the most minimal notification, and
// observers ask for details explicitly thereafter". Here there is not even that -- which
// is why this form is in the file: it marks the end of the axis (see form B for pull with
// a notification).
//
// The list that does exist here is the call sequence in main(), and it is re-created by
// hand on every round.
//
// Build: g++ -std=c++17 -Wall -Wextra form_f_pure_pull.cpp -o /tmp/ff && /tmp/ff

#include <cstdio>

class Sensor {
 public:
  void Set(double value) { value_ = value; }  // no notification, no list, nobody told
  double value() const { return value_; }

 private:
  double value_ = 0.0;
};

class ConsoleDisplay {
 public:
  void Poll(const Sensor& sensor) const {
    std::printf("[console] polls %.1f\n", sensor.value());
  }
};

class AuditDisplay {
 public:
  void Poll(const Sensor& sensor) {
    total_ += sensor.value();
    std::printf("[audit] polls %.1f (total %.1f)\n", sensor.value(), total_);
  }

 private:
  double total_ = 0.0;
};

class AlarmDisplay {
 public:
  void Poll(const Sensor& sensor) const {
    std::printf(sensor.value() > 90.0 ? "[alarm] OVER %.1f\n" : "[alarm] ok %.1f\n",
                sensor.value());
  }
};

int main() {
  Sensor sensor;
  ConsoleDisplay console;
  AuditDisplay audit;
  AlarmDisplay alarm;

  const double kReadings[] = {25.0, 26.5, 27.0, 99.0};
  for (double value : kReadings) {
    sensor.Set(value);
    std::printf("-- cycle %.1f\n", value);
    // The receiver list, such as it is: a hand-written call sequence, rewritten every
    // round, that the sensor never sees.
    console.Poll(sensor);
    audit.Poll(sensor);
    alarm.Poll(sensor);
    std::printf("-- cycle %.1f done\n", value);
  }
  return 0;
}
