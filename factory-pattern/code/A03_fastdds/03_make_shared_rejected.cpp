// 03_make_shared_rejected.cpp
// 【本文件预期编译失败——用于取证，不作为构建目标】
//
// 目的：证明 protected 构造函数会让 std::make_shared / std::make_unique 无法使用，
//       从而解释 Fast-DDS 为什么被迫写成 shared_ptr<T>(new T, custom_deleter)。
//
// 复现命令：
//   g++ -std=c++17 -fsyntax-only 03_make_shared_rejected.cpp
//
// 去掉下面任意一行的注释即可看到对应报错。

#include <memory>

class ProtectedSingleton
{
protected:

    ProtectedSingleton() = default;
    ~ProtectedSingleton() = default;
};

int main()
{
    // ① make_shared：标准库内部要执行 new ProtectedSingleton()，无访问权限
    auto a = std::make_shared<ProtectedSingleton>();

    // ② make_unique：同理
    auto b = std::make_unique<ProtectedSingleton>();

    // ③ 即使只是「构造」而非「析构」，protected 一样拦得住
    (void)a;
    (void)b;
    return 0;
}
