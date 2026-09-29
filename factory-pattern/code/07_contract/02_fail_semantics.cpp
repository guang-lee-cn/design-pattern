// 02_fail_semantics.cpp —— 创建失败时，工厂怎么把这件事告诉调用方？
//
// 四种表达：
//   1. Sensor*          返回 nullptr
//   2. unique_ptr<T>    抛异常
//   3. optional<T>      返回 nullopt（值语义）
//   4. expected<T,E>    返回 unexpected(错误详情)（C++23）
//
// 编译：g++ -std=c++23 -O1 02_fail_semantics.cpp -o 02
// 运行：
//   02 all      四种方式逐条演示失败路径
//   02 crash    演示「忘检查 → 段错误」（单独跑，因为它会终止进程）

#include <cstdio>
#include <expected>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

struct Sensor {
    std::string name;
    // 必须真的访问成员，否则 nullptr 解引用不会崩（实测踩过）
    int read() const { return static_cast<int>(name.size()) + 41; }
};

static bool known(const std::string& t) {
    return t == "temp" || t == "pressure";
}

// ---- 1. 裸指针 + nullptr
Sensor* make_raw(const std::string& t) {
    if (!known(t)) return nullptr;
    return new Sensor{t};
}

// ---- 2. unique_ptr + 异常
std::unique_ptr<Sensor> make_throwing(const std::string& t) {
    if (!known(t)) throw std::invalid_argument("unknown sensor: " + t);
    return std::unique_ptr<Sensor>(new Sensor{t});
}

// ---- 3. optional（值语义）
std::optional<Sensor> make_optional(const std::string& t) {
    if (!known(t)) return std::nullopt;
    return Sensor{t};
}

// ---- 4. expected（值语义 + 错误详情）
std::expected<std::unique_ptr<Sensor>, std::string> make_expected(const std::string& t) {
    if (!known(t)) return std::unexpected(std::string("unknown sensor: ") + t);
    return std::unique_ptr<Sensor>(new Sensor{t});
}

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "all";

    if (mode == "crash") {
        std::printf("=== 忘记检查 nullptr 的后果 ===\n");
        Sensor* p = make_raw("gyro");
        std::printf("make_raw(\"gyro\") 返回 %p\n", static_cast<void*>(p));
        std::printf("调用方直接解引用：\n");
        std::fflush(stdout);
        std::printf("read() = %d\n", p->read());
        return 0;
    }

    std::printf("=== 四种失败表达的行为 ===\n\n");

    std::printf("[1] Sensor*        —— 返回 nullptr\n");
    {
        Sensor* s = make_raw("gyro");
        if (s == nullptr) {
            std::printf("    调用方检查了：s == nullptr\n");
        } else {
            delete s;
        }
        std::printf("    信号强度：无。忘记检查时编译器一声不吭\n");
    }

    std::printf("\n[2] unique_ptr<T>  —— 抛异常\n");
    {
        try {
            auto s = make_throwing("gyro");
            std::printf("    不该走到这里\n");
        } catch (const std::invalid_argument& e) {
            std::printf("    调用方捕获 invalid_argument：%s\n", e.what());
        }
        std::printf("    信号强度：强，但调用点可以完全无视\n");
    }

    std::printf("\n[3] optional<T>    —— 返回 nullopt\n");
    {
        auto s = make_optional("gyro");
        if (!s) {
            std::printf("    调用方检查了：!s\n");
        }
        try {
            s.value();
        } catch (const std::bad_optional_access& e) {
            std::printf("    无视检查强行取值的后果：bad_optional_access（%s）\n", e.what());
        }
        std::printf("    s.has_value() = %d，无堆分配\n",
                    static_cast<int>(s.has_value()));
    }

    std::printf("\n[4] expected<T,E>  —— 返回 unexpected\n");
    {
        auto s = make_expected("gyro");
        if (!s) {
            std::printf("    调用方检查了：!s，错误详情 = %s\n", s.error().c_str());
        }
        try {
            s.value();
        } catch (const std::bad_expected_access<std::string>& e) {
            std::printf("    无视检查强行取值的后果：bad_expected_access（%s）\n",
                        e.error().c_str());
        }
        std::printf("    sizeof(expected) = %zu，sizeof(optional) = %zu\n",
                    sizeof(std::expected<std::unique_ptr<Sensor>, std::string>),
                    sizeof(std::optional<Sensor>));
    }

    return 0;
}
