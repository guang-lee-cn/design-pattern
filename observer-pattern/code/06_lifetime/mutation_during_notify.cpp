// Lifetime debt, case 2 of 3: the list changes while it is being walked.
//
// A receiver that unsubscribes from inside its own callback is the everyday case: the
// widget is going away, so it says "stop sending to me" -- while the producer is right
// in the middle of sending to it.
//
// Three answers to the same question, and this file measures the first two:
//
//   live, indexed      walk the live container by index
//                      -> the receiver that leaves takes a neighbour down with it
//   live, range-for    walk the live container with begin/end captured up front
//                      -> a neighbour is skipped AND another one is notified twice
//   snapshot           copy the list, walk the copy
//                      -> everyone is notified exactly once, and the unsubscribe
//                         takes effect from the next round on
//   forbidden          the kernel's answer, in include/linux/notifier.h:
//                      the _unregister() routines "_must not_ be called from within
//                      the call chain"
//
// None of the first two is reported by a memory tool: every pointer stays valid, the
// container stays consistent. It is a protocol error, not a lifetime error.
//
// Build: g++ -std=c++17 -Wall -Wextra mutation_during_notify.cpp -o /tmp/d6m && /tmp/d6m

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

class Sensor;

class Receiver {
 public:
  explicit Receiver(std::string name) : name_(std::move(name)) {}
  virtual ~Receiver() = default;
  virtual void OnReading(double value) = 0;

  const std::string& name() const { return name_; }
  void set_owner(Sensor* owner) { owner_ = owner; }

 protected:
  Sensor* owner_ = nullptr;
  std::string name_;
};

class Sensor {
 public:
  void Attach(Receiver* r) {
    r->set_owner(this);
    viewers_.push_back(r);
  }

  void Detach(Receiver* r) {
    viewers_.erase(std::remove(viewers_.begin(), viewers_.end(), r), viewers_.end());
    std::printf("      (Detach(%s): list is now %zu)\n", r->name().c_str(), viewers_.size());
  }

  std::size_t viewer_count() const { return viewers_.size(); }

  // (1) Walk the live container, indexed. The loop condition re-reads viewers_.size().
  void TickIndexed(double value) {
    std::printf("-- publish %.1f over %zu viewer(s), live-by-index\n", value, viewers_.size());
    for (std::size_t i = 0; i < viewers_.size(); ++i) {
      std::printf("    -> viewer[%zu] = %s\n", i, viewers_[i]->name().c_str());
      viewers_[i]->OnReading(value);
    }
    std::printf("-- publish %.1f returned\n", value);
  }

  // (2) Walk the live container with begin/end captured up front -- the way most code
  // is actually written.
  void TickRangeFor(double value) {
    std::printf("-- publish %.1f over %zu viewer(s), live-by-range-for\n", value, viewers_.size());
    for (Receiver* r : viewers_) {
      std::printf("    -> about to notify %s\n", r->name().c_str());
      r->OnReading(value);
    }
    std::printf("-- publish %.1f returned\n", value);
  }

  // (3) Walk a copy. Whoever is on the copy gets told exactly once; the Detach applies
  // from the next round on.
  void TickSnapshot(double value) {
    std::printf("-- publish %.1f over %zu viewer(s), snapshot\n", value, viewers_.size());
    const std::vector<Receiver*> targets = viewers_;
    for (Receiver* r : targets) {
      std::printf("    -> about to notify %s\n", r->name().c_str());
      r->OnReading(value);
    }
    std::printf("-- publish %.1f returned (list now %zu)\n", value, viewers_.size());
  }

 private:
  std::vector<Receiver*> viewers_;
};

// A receiver that leaves while the list is being walked.
class LeavesOnFirst : public Receiver {
 public:
  using Receiver::Receiver;
  void OnReading(double value) override {
    std::printf("    [%s] got %.1f -> removing myself from inside the callback\n",
                name_.c_str(), value);
    owner_->Detach(this);
  }
};

class Quiet : public Receiver {
 public:
  using Receiver::Receiver;
  void OnReading(double value) override {
    std::printf("    [%s] got %.1f\n", name_.c_str(), value);
  }
};

namespace {

void Run(const char* label, void (Sensor::*tick)(double)) {
  std::printf("\n===== %s =====\n", label);
  Sensor sensor;
  LeavesOnFirst a("A-leaves");
  Quiet b("B-quiet");
  Quiet c("C-quiet");
  sensor.Attach(&a);
  sensor.Attach(&b);
  sensor.Attach(&c);
  std::printf("  three receivers attached: A-leaves, B-quiet, C-quiet\n");

  for (int round = 0; round < 2; ++round) {
    std::printf("  --- round %d ---\n", round);
    (sensor.*tick)(20.0 + round);
  }
}

}  // namespace

int main() {
  Run("answer 1: walk the live container, by index", &Sensor::TickIndexed);
  Run("answer 2: walk the live container, by range-for", &Sensor::TickRangeFor);
  Run("answer 3: walk a snapshot", &Sensor::TickSnapshot);
  return 0;
}
