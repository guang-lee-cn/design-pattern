// testability_after.cpp —— 痛点 4（后）：产品从外部注入，测试可替换
//
// 编译：g++ -std=c++17 -O0 --coverage testability_after.cpp -o after
// 运行：./after && gcov -b -c after-testability_after.gcno

#include <cstdio>

struct Sensor {
    virtual ~Sensor() = default;
    virtual int read() const = 0;
};

class AlarmService {
public:
    explicit AlarmService(const Sensor& s) : sensor_(s) {}

    bool over_threshold() const {
        if (sensor_.read() > 100) {   // 同一段逻辑，输入改由外部决定
            return true;
        }
        return false;
    }

private:
    const Sensor& sensor_;
};

struct FakeSensor : Sensor {
    int value;
    explicit FakeSensor(int v) : value(v) {}
    int read() const override { return value; }
};

// 同一段测试意图，两个方向都能构造出来
int main() {
    FakeSensor normal(42), over(150);
    AlarmService a(normal), b(over);

    std::printf("[after]  注入  42 → over_threshold() = %d\n", static_cast<int>(a.over_threshold()));
    std::printf("[after]  注入 150 → over_threshold() = %d\n", static_cast<int>(b.over_threshold()));
    std::printf("[after]  可构造的输入 : 2 种（可任意扩展）\n");
    std::printf("[after]  结论         : 两侧都能覆盖，生产代码改动 0 行\n");
    return 0;
}
