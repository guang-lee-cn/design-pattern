#include "loggers.h"

#include <fstream>
#include <utility>

namespace fp {

FileLogger::FileLogger(FileLoggerConfig config) : config_(std::move(config)) {
    // 刻意做得极简：demo 只需要"写了几行"这个事实，
    // 不需要滚动、刷盘线程这些真实文件日志器才有的复杂度。
    std::ofstream out(config_.path,
                      config_.append ? std::ios::app : std::ios::trunc);
    out << "[FileLogger] open " << config_.path
        << " (append=" << (config_.append ? "true" : "false")
        << ", flush_interval_ms=" << config_.flush_interval_ms
        << ", encoding=" << config_.encoding << ")\n";
}

void FileLogger::info(const std::string& message) {
    std::ofstream out(config_.path, std::ios::app);
    out << "[info]  " << message << '\n';
    ++lines_written_;
    // 刻意不往 stdout 写 —— 所以 --kind file 时控制台是安静的。
}

void FileLogger::warn(const std::string& message) {
    std::ofstream out(config_.path, std::ios::app);
    out << "[warn]  " << message << '\n';
    ++lines_written_;
}

}  // namespace fp
