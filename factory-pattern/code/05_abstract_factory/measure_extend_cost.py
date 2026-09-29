#!/usr/bin/env python3
"""测量：抽象工厂下"新增一个产品类型"要改多少处 —— 用编译器报错数当量化指标。
做法：生成 N 个族的抽象工厂代码，在接口里加一个 create_spi()，
      但不更新具体工厂 → 编译器逐族报错。错误数即"必须改动的具体工厂数"。
"""
import subprocess
import tempfile
import os

TEMPLATE = """
#include <memory>
#include <string>
class Uart  {{ public: virtual ~Uart() = default; }};
class Gpio  {{ public: virtual ~Gpio() = default; }};
class Timer {{ public: virtual ~Timer() = default; }};

class BspFactory {{
public:
    virtual ~BspFactory() = default;
    virtual std::unique_ptr<Uart>  create_uart()  = 0;
    virtual std::unique_ptr<Gpio>  create_gpio()  = 0;
    virtual std::unique_ptr<Timer> create_timer() = 0;
{extra_decl}}};

{classes}
int main() {{
{instantiate}    return 0;
}}
"""

INSTANTIATE = "    auto p{n} = std::make_unique<BspFactory{n}>();  (void)p{n};\n"

FAMILY = """
class Uart{n}  : public Uart  {{}};
class Gpio{n}  : public Gpio  {{}};
class Timer{n} : public Timer {{}};
class BspFactory{n} : public BspFactory {{
public:
    std::unique_ptr<Uart>  create_uart()  override {{ return std::make_unique<Uart{n}>(); }}
    std::unique_ptr<Gpio>  create_gpio()  override {{ return std::make_unique<Gpio{n}>(); }}
    std::unique_ptr<Timer> create_timer() override {{ return std::make_unique<Timer{n}>(); }}
}};
"""


def build(n_families: int, add_product: bool) -> str:
    extra = "    virtual std::unique_ptr<Uart>  create_spi()  = 0;\n" if add_product else ""
    classes = "".join(FAMILY.format(n=i) for i in range(1, n_families + 1))
    instantiate = "".join(INSTANTIATE.format(n=i) for i in range(1, n_families + 1))
    return TEMPLATE.format(extra_decl=extra, classes=classes, instantiate=instantiate)


def count_errors(src: str) -> int:
    with tempfile.NamedTemporaryFile("w", suffix=".cpp", delete=False) as f:
        f.write(src)
        path = f.name
    try:
        r = subprocess.run(["g++", "-std=c++17", "-fsyntax-only", path],
                           capture_output=True, text=True)
        return sum(1 for line in r.stderr.splitlines() if "error:" in line)
    finally:
        os.unlink(path)


print(f"{'族数 N':<8}{'基线错误':<10}{'加一个产品类型后':<18}{'增量'}")
for n in (2, 4, 8, 16):
    base = count_errors(build(n, add_product=False))
    after = count_errors(build(n, add_product=True))
    print(f"{n:<10}{base:<12}{after:<20}{after - base}")
