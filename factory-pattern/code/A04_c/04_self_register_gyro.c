/*
 * A04 / 04 —— 静态库成员里的自注册
 *
 * 本文件单独编译成 .o，再打进 libsensors.a。
 * build.sh 会用两种链接方式链接同一个主程序（03），观察段内容的差异：
 *   A. 普通链接     -> gyro 不在段里（该 .o 无符号被引用，链接器不拉它）
 *   B. --whole-archive -> gyro 在段里
 *
 * 这是"零改动自注册"的隐藏账单：机制本身没问题，但【链接模型】会咬人。
 */
#include "sensor_registry.h"

static int gyro_read(void) { return 700; }

REGISTER_DRIVER("gyro", gyro_read);
