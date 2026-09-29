#include <cstdio>
#include <exception>
#include <string>

void report(const std::string&);       // business.cpp —— 经工厂
void report_old(const std::string&);   // business_old.cpp —— 直接 new

int main() {
    std::printf("== 新世界：business.cpp 经工厂 ==\n");
    report("temp");
    report("pressure");
    try {
        report("humidity");   // 未注册类型：工厂是唯一报错点
    } catch (const std::exception& e) {
        std::printf("  expected error: %s\n", e.what());
    }

    std::printf("== 旧世界：business_old.cpp 直接 new ==\n");
    report_old("temp");
    report_old("pressure");
    return 0;
}
