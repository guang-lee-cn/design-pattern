/* mod_log.c —— 一个模块：自己定义实例，不注册、不登记 */
#include "modules.h"

static int log_init(void)        { return 300; }
static int log_handle(int req)   { return req + 3; }

const module_t mod_log = { "log", log_init, log_handle };
