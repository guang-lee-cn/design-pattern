// testability_before.cpp —— 痛点 4（前）：产品在函数体内部构造，测试无法替换
//
// 编译：g++ -std=c++17 -O0 --coverage testability_before.cpp -o before
// 运行：./before && gcov -b -c before-testability_before.gcno

#include <cstdio>

struct RealSensor {
    int read() const { return 42; }   // 读数写死在实现里，测试改不了它
};

class AlarmService {
public:
    bool over_threshold() const {
        RealSensor s;                 // ← 产品在函数体里造出来，没有接缝
        if (s.read() > 100) {         // ← 分支真实存在，但 true 侧永远走不到
            return true;
        }
        return false;
    }
};

// 测试代码：想覆盖「读数超阈值 → 报警」这一侧
int main() {
    AlarmService svc;
    const bool alarmed = svc.over_threshold();

    std::printf("[before] over_threshold() = %d\n", static_cast<int>(alarmed));
    std::printf("[before] 想覆盖的分支 : read() > 100  →  true\n");
    std::printf("[before] 可构造的输入 : 1 种（read() 恒返回 42）\n");
    std::printf("[before] 结论         : true 侧不可达，除非改生产代码\n");
    return 0;
}
