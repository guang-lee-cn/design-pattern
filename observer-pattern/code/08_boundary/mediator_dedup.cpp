// Why a ChangeManager is worth its indirection -- and only sometimes.
//
// GoF, Observer, Implementation:
//
//   "A DAGChangeManager is preferable to a SimpleChangeManager when an observer observes
//    more than one subject. In that case, a change in two or more subjects might cause
//    redundant updates. The DAGChangeManager ensures the observer receives just one update.
//    SimpleChangeManager is fine when multiple updates aren't an issue."
//
// One observer, two subjects, one transaction that changes both. The receiver does not
// move and the data does not change -- only the number of times it is woken up does.
//
// Build: g++ -std=c++17 -Wall -Wextra mediator_dedup.cpp -o /tmp/fm && /tmp/fm

#include <cstdio>
#include <string>
#include <vector>

class DashboardDisplay {
 public:
  void OnChange(const std::string& subject) {
    ++updates_;
    std::printf("[dashboard] updated for %s\n", subject.c_str());
  }

  int updates() const { return updates_; }
  void Reset() { updates_ = 0; }

 private:
  int updates_ = 0;
};

// Naive strategy: every subject change is one notification.
class SimpleChangeManager {
 public:
  explicit SimpleChangeManager(DashboardDisplay& display) : display_(display) {}

  void MarkChanged(const std::string& subject) { display_.OnChange(subject); }

 private:
  DashboardDisplay& display_;
};

// DAG strategy: changes are accumulated and delivered once, when the transaction ends.
class DagChangeManager {
 public:
  explicit DagChangeManager(DashboardDisplay& display) : display_(display) {}

  void MarkChanged(const std::string& subject) { changed_.push_back(subject); }

  void Commit() {
    if (changed_.empty()) return;
    // One wakeup for the whole transaction, no matter how many subjects moved.
    display_.OnChange(changed_.front() + " + " + std::to_string(changed_.size() - 1) +
                      " more");
    changed_.clear();
  }

 private:
  DashboardDisplay& display_;
  std::vector<std::string> changed_;
};

int main() {
  DashboardDisplay dashboard;
  const char* kSubjects[] = {"temperature", "humidity"};

  std::printf("== one transaction: change %s and %s ==\n", kSubjects[0], kSubjects[1]);
  std::printf("-- the producer holds only a reference to its manager --\n\n");

  std::printf("SimpleChangeManager\n");
  SimpleChangeManager simple(dashboard);
  for (const char* subject : kSubjects) {
    simple.MarkChanged(subject);
  }
  const int simple_updates = dashboard.updates();

  dashboard.Reset();
  std::printf("\nDAGChangeManager\n");
  DagChangeManager dag(dashboard);
  for (const char* subject : kSubjects) {
    dag.MarkChanged(subject);
  }
  dag.Commit();
  const int dag_updates = dashboard.updates();

  std::printf("\n== one transaction, two subjects changed ==\n");
  std::printf("SimpleChangeManager : %d update(s)\n", simple_updates);
  std::printf("DAGChangeManager    : %d update(s)\n", dag_updates);
  return 0;
}
