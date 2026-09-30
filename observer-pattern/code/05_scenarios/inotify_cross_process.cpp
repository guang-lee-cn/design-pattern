// Scenario 3 of 3: file change notification, measured across a process boundary.
//
// The publisher writes a file and exits. The watcher is a DIFFERENT process and
// learns about it from the kernel. On this platform, space decoupling is not a
// design choice -- it is the only shape available.
//
// Two things get measured here that the in-process ladder cannot show:
//
//   1. The publisher has already exited by the time the notification is read.
//      In-process, "asynchronous" still means "another thread in my address space".
//      Here it means "another address space entirely".
//
//   2. A watcher registered AFTER the write receives nothing. Unlike the layer-4
//      queue, this bus keeps no backlog: no queue depth, no replay. A late
//      subscriber is simply not a subscriber.
//
// Build/run: ./run_scenarios.sh

#include <sys/inotify.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

constexpr int kEventSize = static_cast<int>(sizeof(struct inotify_event));
constexpr int kNameMax = 255;

void Say(const std::string& text) {
  std::fputs(text.c_str(), stdout);
  std::fputc('\n', stdout);
  std::fflush(stdout);
}

// Writes the file, announces both sides of the write, then exits.
[[noreturn]] void PublisherProcess(const std::string& path) {
  Say("-- publish: writing " + path);
  Say("-- publish returned (file written, no consumer has run)");

  FILE* file = std::fopen(path.c_str(), "w");
  if (file == nullptr) {
    std::fprintf(stderr, "publisher: fopen failed: %s\n", std::strerror(errno));
    _exit(1);
  }
  std::fputs("interval=30\n", file);
  std::fclose(file);

  _exit(0);
}

void DrainEvents(int fd, const std::string& label, const std::string& dir) {
  char buffer[kEventSize + kNameMax + 1];
  const ssize_t count = ::read(fd, buffer, sizeof(buffer));
  if (count <= 0) {
    Say("[" + label + "] no event received");
    return;
  }
  for (int offset = 0; offset < count;) {
    const auto* event = reinterpret_cast<const struct inotify_event*>(buffer + offset);
    const char* action = (event->mask & IN_MODIFY)  ? "IN_MODIFY"
                         : (event->mask & IN_CREATE) ? "IN_CREATE"
                                                     : "other";
    Say("[" + label + "] event: " + action + " " + dir + "/" + event->name);
    offset += kEventSize + static_cast<int>(event->len);
  }
}

}  // namespace

int main() {
  setvbuf(stdout, nullptr, _IOLBF, 0);

  const std::string dir = "/tmp/observer05-watch";
  const std::string path = dir + "/config.txt";
  const std::string late_path = dir + "/late.txt";

  Say("=== D. File change across a process boundary ===");

  // Start from a known state, so that every run produces the same event set.
  ::unlink(path.c_str());
  ::unlink(late_path.c_str());

  const int inotify_fd = ::inotify_init1(IN_CLOEXEC);
  if (inotify_fd < 0) {
    std::fprintf(stderr, "inotify_init1 failed: %s\n", std::strerror(errno));
    return 1;
  }
  const int wd = ::inotify_add_watch(inotify_fd, dir.c_str(), IN_MODIFY | IN_CREATE);

  const pid_t child = ::fork();
  if (child == 0) {
    ::close(inotify_fd);
    PublisherProcess(path);
  }
  ::waitpid(child, nullptr, 0);
  Say("-- publisher process pid=" + std::to_string(child) + " has exited");
  Say("-- watcher now reads what the kernel queued:");

  DrainEvents(inotify_fd, "watcher-pid=" + std::to_string(::getpid()), dir);
  ::inotify_rm_watch(inotify_fd, wd);

  Say("");
  Say("=== E. A watcher registered after the write ===");
  Say("-- publish: writing " + late_path);
  Say("-- publish returned");
  FILE* file = std::fopen(late_path.c_str(), "w");
  if (file != nullptr) {
    std::fputs("late\n", file);
    std::fclose(file);
  }

  // Non-blocking on purpose: "nothing arrives" is the result being measured.
  const int late_fd = ::inotify_init1(IN_CLOEXEC | IN_NONBLOCK);
  ::inotify_add_watch(late_fd, dir.c_str(), IN_MODIFY | IN_CREATE);
  Say("-- late watcher registered; reading now:");
  DrainEvents(late_fd, "late-watcher", dir);
  Say("   ^ no backlog, no replay: the write happened before the watch existed");

  ::close(late_fd);
  ::close(inotify_fd);
  return 0;
}
