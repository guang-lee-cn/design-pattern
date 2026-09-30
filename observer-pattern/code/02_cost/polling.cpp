// 02_cost · 路线二「轮询」—— 显示端按固定周期去问传感器：变了吗？变了吗？
//
//   编译运行： g++ -std=c++17 -O2 -Wall polling.cpp -o polling && ./polling
//
// 三件事都能用数字说清：
//   A. 延迟 vs 空转：缩短周期治延迟但烧 CPU，放长周期省 CPU 但延迟涨
//   B. 尖峰漏报：周期一旦长过瞬变的持续时间，漏报概率 = (周期 - 持续时间) / 周期
//   C. 真实 CPU：最粗暴的忙轮询占满一个核，而这段时间里值一次都没变
//
// A 与 B 用确定性时间轴（1 tick = 10 ms），数字可复现、不含机器运气。
#include <chrono>
#include <cstdio>
#include <ctime>
#include <string>

static constexpr double kTickMs       = 10.0;
static constexpr double kInitialValue = 20.0;

// 按「字符数」（不是字节数）左对齐填充 —— 中文是多字节，printf 的 %-12s 会撑歪列宽
static std::string pad(const std::string& s, int w) {
    int n = 0;
    for (unsigned char c : s) if ((c & 0xC0) != 0x80) ++n;
    return s + std::string(n < w ? w - n : 0, ' ');
}

static void row(const char* a, const char* b, const char* c, const char* d) {
    std::printf("%s%s%s%s\n", pad(a, 13).c_str(), pad(b, 13).c_str(),
                pad(c, 19).c_str(), pad(d, 19).c_str());
}

// ---------------------------------------------------------------------------
// A. 延迟 vs 空转
// ---------------------------------------------------------------------------
// 时间轴共 60 tick；值只在 3 个时刻变，且刻意落在 tick 之间，
// 免得与轮询节拍对齐（对齐了延迟恒为 0，看不出问题）。
static const double kChangeAt[3] = {10.5, 25.5, 40.5};
static const double kChangeTo[3] = {23.5, 24.1, 25.0};

struct Timeline {
    double value      = kInitialValue;
    double changed_at = 0.0;    // 当前这个值是什么时候变过来的
    int    applied    = 0;

    void advance_to(double now) {
        while (applied < 3 && now >= kChangeAt[applied]) {
            value      = kChangeTo[applied];
            changed_at = kChangeAt[applied];
            ++applied;
        }
    }
};

static void experiment_a() {
    constexpr int kDisplays = 2;   // 两个显示端，各自独立轮询
    constexpr int kTicks    = 60;

    std::printf("A. 延迟 vs 空转（时间轴 %.0f ms，期间值只变了 3 次，两个显示端）\n\n",
                kTicks * kTickMs);
    row("轮询周期", "读数次数", "最大延迟", "平均延迟");
    std::printf("%s\n", std::string(64, '-').c_str());

    for (int period : {1, 2, 4, 8, 20}) {
        Timeline tl;
        // 显示端一开始就知道当前读数，所以「第一次读」不算发现变化
        double seen[kDisplays];
        for (int d = 0; d < kDisplays; ++d) seen[d] = kInitialValue;

        int    reads = 0, lat_n = 0;
        double lat_max = 0.0, lat_sum = 0.0;

        for (int tick = 1; tick <= kTicks; ++tick) {
            const double now = tick;
            tl.advance_to(now);
            if (tick % period != 0) continue;          // 没到本显示端的周期
            for (int d = 0; d < kDisplays; ++d) {
                ++reads;                               // ← 这一问，就是空转
                if (tl.value == seen[d]) continue;
                seen[d] = tl.value;
                const double lat = now - tl.changed_at;
                lat_sum += lat; ++lat_n;
                if (lat > lat_max) lat_max = lat;
            }
        }

        char pa[32], pb[32], pc[48], pd[48];
        std::snprintf(pa, sizeof pa, "%d tick", period);
        std::snprintf(pb, sizeof pb, "%d", reads);
        std::snprintf(pc, sizeof pc, "%.1f tick (%.0f ms)", lat_max, lat_max * kTickMs);
        std::snprintf(pd, sizeof pd, "%.1f tick (%.0f ms)", lat_sum / lat_n, lat_sum / lat_n * kTickMs);
        row(pa, pb, pc, pd);
    }
    std::printf("\n→ 延迟上限 ≈ 轮询周期；读数次数 ∝ 1/周期。两个方向都在变坏，没有中间的好点。\n");
}

// ---------------------------------------------------------------------------
// B. 尖峰漏报
// ---------------------------------------------------------------------------
// 一次尖峰：值在 2 tick 内升上去又落回来（如温度瞬时过冲）。
// 轮询能不能看见它，取决于「尖峰窗口里恰好有没有一个轮询时刻」——
// 也就是相位，而相位是随机的。这里把相位扫一遍，统计漏掉的比例。
static void experiment_b() {
    constexpr int kSpikeTicks = 2;    // 尖峰持续 2 tick = 20 ms
    constexpr int kPhases     = 5;    // 每个周期扫 5 个周期长度的相位

    std::printf("\nB. 尖峰漏报（尖峰持续 %d tick = %.0f ms，扫描 %d 个周期长度的相位）\n\n",
                kSpikeTicks, kSpikeTicks * kTickMs, kPhases);
    row("轮询周期", "试验次数", "漏报次数", "漏报率");
    std::printf("%s\n", std::string(64, '-').c_str());

    for (int period : {1, 2, 4, 8, 20}) {
        const int trials = period * kPhases;
        int missed = 0;
        for (int i = 0; i < trials; ++i) {
            const double spike_at = 10.5 + i;      // 尖峰起始时刻（逐 tick 挪相位）
            bool seen = false;
            for (int t = period; t <= 200; t += period) {
                if (t >= spike_at && t < spike_at + kSpikeTicks) { seen = true; break; }
            }
            if (!seen) ++missed;
        }
        char pa[32], pb[32], pc[32], pd[32];
        std::snprintf(pa, sizeof pa, "%d tick", period);
        std::snprintf(pb, sizeof pb, "%d", trials);
        std::snprintf(pc, sizeof pc, "%d", missed);
        std::snprintf(pd, sizeof pd, "%.0f%%", 100.0 * missed / trials);
        row(pa, pb, pc, pd);
    }
    std::printf("\n→ 周期 ≤ 尖峰时长：一次都漏不掉；周期一长过它，漏报率 = (周期 - 尖峰时长) / 周期。\n");
    std::printf("  漏不漏还取决于相位 —— 同样的代码，这一次报警响了，下一次可能就没响。\n");
}

// ---------------------------------------------------------------------------
// C. 真实 CPU
// ---------------------------------------------------------------------------
// 最粗暴的轮询形态：不 sleep，一直问。平稳一点的写法会睡到下一个周期，
// 那时代价换成「定时器唤醒 + 总线/寄存器访问」，但 A、B 两份账一分不少。
static void experiment_c() {
    constexpr double kWallSeconds = 0.2;
    volatile int sink = 0;
    int iter = 0;

    const auto wall0 = std::chrono::steady_clock::now();
    const std::clock_t cpu0 = std::clock();
    for (;;) {
        sink += 1;                                  // 「读一次传感器」
        ++iter;
        const auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration<double>(now - wall0).count() >= kWallSeconds) break;
    }
    const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - wall0).count();
    const double cpu  = static_cast<double>(std::clock() - cpu0) / CLOCKS_PER_SEC;

    // 不报「占了几成核」：进程 CPU 时间与墙钟时间由不同时钟源给出，
    // 虚拟化环境里会有几个百分点的偏差，报出来反而像是算错了。
    std::printf("\nC. 真实 CPU（本机实测；绝对数随机器变，结论不变）\n\n");
    std::printf("   墙钟 %.0f ms 内问了 %d 次（约 %.0f 万次/秒）\n",
                wall * 1000, iter, iter / wall / 10000.0);
    std::printf("   进程 CPU 时间 %.0f ms —— 与墙钟时间基本相同，即这一个核被占满\n", cpu * 1000);
    std::printf("   同一段时间里，值变化次数：0\n");
    std::printf("   —— 不管有没有变化，它都在问。这就是「空转」。\n");
    (void)sink;
}

int main() {
    std::printf("================ 路线二 轮询：三份账单 ================\n\n");
    experiment_a();
    experiment_b();
    experiment_c();
    return 0;
}
