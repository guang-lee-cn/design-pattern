/* mod_http.c —— 一个模块：自己定义实例，不注册、不登记 */
#include "modules.h"

static int http_init(void)        { return 200; }
static int http_handle(int req)   { return req + 2; }

const module_t mod_http = { "http", http_init, http_handle };
