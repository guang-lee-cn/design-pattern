/* mod_core.c —— 一个模块：自己定义实例，不注册、不登记 */
#include "modules.h"

static int core_init(void)        { return 100; }
static int core_handle(int req)   { return req + 1; }

const module_t mod_core = { "core", core_init, core_handle };
