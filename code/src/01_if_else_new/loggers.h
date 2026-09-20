#pragma once

#include "fp/logger.h"

#include <string>

namespace fp {

/// 文件日志器的构造参数。
///
/// 这个结构体是第 1 篇的"道具"：它代表**具体类的构造过程**。
/// 构造过程一旦变化，谁 include 了本头文件，谁就要重编 ——
/// 这正是「直接 new」和「工厂」之间的第一处差别。
///
/// 特别注意：它带默认成员初始值，所以仍然是一个聚合体（aggregate），
/// 可以直接写 FileLoggerConfig{"app.log", true, 100, "utf-8"}。
struct FileLoggerConfig {
    std::string path;
    bool        append            = false;
    int         flush_interval_ms = 0;
    std::string encoding          = "utf-8";
};

/// 写文件的具体日志器。
///
/// 它定义在这里，而不是在 include/fp/ 下 —— 「全系列共享的抽象接口」和
/// 「某个具体实现」是两件事，第 1 篇要演的正是这个区别：
/// 具体实现被谁 include，谁就和它绑定了编译依赖。
class FileLogger final : public Logger {
public:
    explicit FileLogger(FileLoggerConfig config);

    void info(const std::string& message) override;
    void warn(const std::string& message) override;

    /// 本次进程内写出的日志行数，供 demo 与断言使用。
    int lines_written() const { return lines_written_; }

private:
    FileLoggerConfig config_;
    int              lines_written_ = 0;
};

}  // namespace fp
