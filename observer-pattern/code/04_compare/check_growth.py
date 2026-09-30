#!/usr/bin/env python3
"""第 4 节 · 场景长大时，编译器会拦下多少个既有类？

同一份传感器场景，加第二种事件（传感器故障）。四层各自的「加」法不一样，
于是「谁被牵连」也不一样。判据不靠人读，靠编译器：把改过的副本编一遍，看它拦下谁。

只有第 2 / 3 / 4 层进这个实验。第 1 层没有契约可加 —— 它连「接收方」这个概念都没有，
「牵连接收方」这件事在它身上无从发生；它付的代价在别处（生产者自己得知道有第二种事件），
那一笔记在正文里，不在这里。

变体写进临时目录时保留原文件名，这样编译器的报错直接指向被牵连的那个源文件。

用法（在本目录下）：python3 check_growth.py
"""
import pathlib
import re
import shutil
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
OUT = pathlib.Path("/tmp/observer_04_growth")
CXX = "g++"

ERROR_RE = re.compile(r"^(.+?):\d+:\d+: error: (.*)$")
# 编译器把类名套在单引号里，本机是 U+2018 / U+2019
QUOTED_RE = re.compile("[\u2018']([A-Z][A-Za-z0-9_]*)[\u2019']")


def compile_variant(name, source):
    """把变体编一遍，返回 (是否通过, [(源文件, 错误信息)])。"""
    OUT.mkdir(parents=True, exist_ok=True)
    path = OUT / f"{name}.cpp"
    path.write_text(source)
    cmd = [CXX, "-std=c++17", "-Wall", "-Wextra", "-fsyntax-only", "-pthread", str(path)]
    proc = subprocess.run(cmd, capture_output=True, text=True)
    errors = []
    for line in proc.stderr.splitlines():
        m = ERROR_RE.match(line)
        if m:
            errors.append((pathlib.Path(m.group(1)).name, m.group(2)))
    return proc.returncode == 0, errors


def add_second_event_to_observer(source):
    """观察者层的加法：往契约里再加一条纯虚。"""
    anchor = "  virtual void OnReading(double value) = 0;"
    assert anchor in source, "锚点没找到：layer2 的 Observer 契约"
    return source.replace(anchor, anchor + "\n  virtual void OnError(int code) = 0;", 1)


def add_second_event_to_bus(source):
    """中介层的加法：多订一个主题，既有代码一行不动。"""
    anchor = '  Sensor sensor(bus, "temperature");'
    assert anchor in source, "锚点没找到：main 里的 Sensor 构造"
    return source.replace(
        anchor, '  bus.Subscribe("sensor_error", [](double v) { (void)v; });\n' + anchor, 1
    )


def main():
    rows = []

    src = (HERE / "layer2_observer.cpp").read_text()
    ok, errors = compile_variant("layer2_observer", add_second_event_to_observer(src))
    rows.append(("layer2_observer.cpp", "契约里加一条纯虚", ok, errors))

    for name in ("layer3_eventbus.cpp", "layer4_pubsub.cpp"):
        src = (HERE / name).read_text()
        ok, errors = compile_variant(name[:-4], add_second_event_to_bus(src))
        rows.append((name, "多订一个主题", ok, errors))

    print("### 加第二种事件（传感器故障）后，编译器拦下了谁")
    print()
    print("  %-24s %-20s %-8s %s" % ("文件", "加法", "结果", "被点名的既有类"))
    for name, how, ok, errors in rows:
        named = sorted({q for _, msg in errors for q in QUOTED_RE.findall(msg)})
        print(
            "  %-24s %-20s %-8s %s"
            % (name, how, "PASS" if ok else "FAIL", ", ".join(named) or "-")
        )
    print()
    for name, how, ok, errors in rows:
        if ok:
            continue
        print(f"--- {name} 的报错原文（共 {len(errors)} 条）---")
        for f, msg in errors:
            print(f"  {f}: error: {msg}")
        print()

    shutil.rmtree(OUT, ignore_errors=True)


if __name__ == "__main__":
    sys.exit(main())
