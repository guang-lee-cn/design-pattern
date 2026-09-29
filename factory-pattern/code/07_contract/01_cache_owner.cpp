// 01_cache_owner.cpp —— 工厂缓存实例时，所有权到底归谁？
//
// 场景：按名字取传感器。创建成本高，工厂内部缓存已建实例。
// 三种返回方式，调用方做同一件「看起来很正常」的事。
//
// 编译：g++ -std=c++17 -O1 -g -fsanitize=address 01_cache_owner.cpp -o 01
// 运行：
//   01 a    裸指针 + 调用方 delete     → 工厂留下悬垂指针
//   01 a2   裸指针 + 调用方不 delete   → 泄漏
//   01 b    shared_ptr 缓存（工厂强持有）→ 正常，但生命周期被工厂钉死
//   01 b2   weak_ptr 缓存（工厂弱持有） → 正常，且没人用时可自动释放
//   01 c    返回引用                     → 调用方无法越权释放，也无法持有

#include <cstdio>
#include <map>
#include <memory>
#include <string>

static int g_alive = 0;

struct Sensor {
    std::string name;
    explicit Sensor(std::string n) : name(std::move(n)) {
        ++g_alive;
        std::printf("  [构造] %s\n", name.c_str());
    }
    ~Sensor() {
        --g_alive;
        std::printf("  [析构] %s\n", name.c_str());
    }
    // 注意：这里必须真的访问成员。若写成 return 42;，编译器不会解引用 this，
    // UAF 与空指针都会被悄悄掩盖 —— 实测踩过。
    int read() const { return static_cast<int>(name.size()) + 41; }
};

// ---- (a) 返回裸指针，工厂内部缓存裸指针
class RawCacheFactory {
public:
    Sensor* get(const std::string& type) {
        auto it = cache_.find(type);
        if (it != cache_.end()) {
            std::printf("  命中缓存\n");
            return it->second;
        }
        Sensor* p = new Sensor(type);
        cache_[type] = p;
        std::printf("  新建并写入缓存\n");
        return p;
    }

private:
    std::map<std::string, Sensor*> cache_;
};

// ---- (b) 返回 shared_ptr，工厂内部强持有
class SharedCacheFactory {
public:
    std::shared_ptr<Sensor> get(const std::string& type) {
        auto it = cache_.find(type);
        if (it != cache_.end()) {
            std::printf("  命中缓存\n");
            return it->second;
        }
        auto p = std::make_shared<Sensor>(type);
        cache_[type] = p;
        std::printf("  新建并写入缓存\n");
        return p;
    }

private:
    std::map<std::string, std::shared_ptr<Sensor>> cache_;
};

// ---- (b2) 返回 shared_ptr，工厂内部弱持有
class WeakCacheFactory {
public:
    std::shared_ptr<Sensor> get(const std::string& type) {
        auto it = cache_.find(type);
        if (it != cache_.end()) {
            if (auto sp = it->second.lock()) {
                std::printf("  命中缓存（对象仍存活）\n");
                return sp;
            }
            std::printf("  缓存已失效，重建\n");
        }
        auto p = std::make_shared<Sensor>(type);
        cache_[type] = p;
        std::printf("  新建并写入缓存\n");
        return p;
    }

private:
    std::map<std::string, std::weak_ptr<Sensor>> cache_;
};

// ---- (c) 返回引用，工厂独占持有
class RefCacheFactory {
public:
    const Sensor& get(const std::string& type) {
        auto it = cache_.find(type);
        if (it != cache_.end()) {
            std::printf("  命中缓存\n");
            return *it->second;
        }
        auto p = std::make_unique<Sensor>(type);
        cache_[type] = std::move(p);
        std::printf("  新建并写入缓存\n");
        return *cache_[type];
    }

private:
    std::map<std::string, std::unique_ptr<Sensor>> cache_;
};

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "a";
    std::printf("=== 模式 %s ===\n", mode.c_str());

    if (mode == "a") {
        RawCacheFactory f;
        Sensor* s = f.get("temp");
        s->read();
        std::printf("调用方执行 delete —— 它认为自己拥有这个对象\n");
        delete s;
        std::printf("工厂再次取同名对象：\n");
        Sensor* s2 = f.get("temp");
        std::printf("  拿到指针 %p，调用 read()...\n", static_cast<void*>(s2));
        std::printf("  read() = %d\n", s2->read());
    } else if (mode == "a2") {
        RawCacheFactory f;
        std::printf("调用方取用一次\n");
        f.get("temp")->read();
        std::printf("工厂再次取同名对象：\n");
        Sensor* again = f.get("temp");
        std::printf("  拿到指针 %p\n", static_cast<void*>(again));
        std::printf("调用方没有 delete —— 它不确定该不该\n");
    } else if (mode == "b") {
        {
            SharedCacheFactory f;
            auto s = f.get("temp");
            std::printf("调用方拿到 shared_ptr，use_count = %ld\n", s.use_count());
            s->read();
        }
        std::printf("调用方作用域结束");
    } else if (mode == "b2") {
        WeakCacheFactory f;
        {
            auto s = f.get("temp");
            s->read();
        }
        std::printf("调用方作用域结束\n");
        auto s2 = f.get("temp");
        std::printf("再取一次：use_count = %ld\n", s2.use_count());
    } else if (mode == "c") {
        RefCacheFactory f;
        const Sensor& r = f.get("temp");
        std::printf("  read() = %d\n", r.read());
        // delete &r;   // ← 取消注释会编译失败：绑定到 const 引用
        std::printf("引用既不能释放，也不能存起来带出工厂\n");
    } else {
        std::printf("未知模式\n");
        return 1;
    }

    std::printf("存活对象数 = %d\n", g_alive);
    return 0;
}
