// fanout / 组合根
#include "display.h"
#include "sensor.h"

void module_a(const TempSensor&);
void module_b(const TempSensor&);
void module_c(const TempSensor&);

int main() {
    TempSensor sensor;
    NumberDisplay number;
    sensor.attach(&number);

    sensor.set(23.5);
    module_a(sensor);
    module_b(sensor);
    module_c(sensor);
    return 0;
}
