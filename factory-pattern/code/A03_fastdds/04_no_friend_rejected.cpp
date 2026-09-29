// 04_no_friend_rejected.cpp
// 【本文件预期编译失败——用于取证，不作为构建目标】
//
// 目的：证明 RPC 侧真正实现 delete_service / delete_service_requester 时
//       会遇到的一道硬门槛——外部类拿不到 protected 析构的访问权。
//
// 复现命令：
//   g++ -std=c++17 -fsyntax-only 04_no_friend_rejected.cpp
//
// 背景：Fast-DDS 当前的 delete_* 是空壳，函数体里只有一句日志，没有 delete 语句，
//       所以现在编译得过。一旦把 delete 补上，本文件的报错就会真实发生。
//       上游给出的两条合法出路见文档：friend 授权 / protected virtual 析构。

#include <cstdio>

// 复刻 include/fastdds/dds/rpc/Service.hpp 的析构策略（protected 非虚）
class Service
{
public:

    virtual const char* name() const = 0;

protected:

    ~Service() = default;          // 没有 friend 声明
};

// 复刻 src/cpp/fastdds/domain/DomainParticipantImpl.cpp 的销毁入口
class DomainParticipantImpl
{
public:

    static int delete_service(const Service* service)
    {
        delete service;            // <- 无关类，无访问权：编译失败
        return 0;
    }
};

int main()
{
    std::printf("%d\n", DomainParticipantImpl::delete_service(nullptr));
    return 0;
}
