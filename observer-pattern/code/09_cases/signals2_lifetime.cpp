// Case 2 of 4: Boost.Signals2 1.83, measured on the real library (header-only).
//
// Section 06-生命周期负债.md listed six answers to "who invalidates the IOU" and
// picked weak reference + lazy cleanup as the only one you cannot forget. Signals2
// is that answer, built into a library. This file measures what it buys and what it
// costs, on the same questions the Qt case asks.
//
//   A1. one tracked receiver dies   -> does the slot stop, is the count honest
//   A2. two tracked receivers       -> how many of them have to die
//   B.  an untracked receiver dies  -> the control group
//   C1. a slot disconnects a sibling, mid-walk
//   C2. a slot disconnects itself, mid-walk
//   D1. scoped_connection
//   D2. shared_connection_block
//   E.  the price of one connection, counted by replacing global operator new
//
// Build/run: ./run_cases.sh

#include <new>

#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace s2 = boost::signals2;

// ---- replacement global allocator: used in section E ----------------------
static long g_alloc = 0;
static long g_free = 0;

void* operator new(std::size_t n) {
  ++g_alloc;
  void* p = std::malloc(n ? n : 1);
  if (p == nullptr) throw std::bad_alloc();
  return p;
}

void* operator new[](std::size_t n) {
  return ::operator new(n);
}

void operator delete(void* p) noexcept {
  if (p != nullptr) ++g_free;
  std::free(p);
}

void operator delete[](void* p) noexcept {
  ::operator delete(p);
}

void operator delete(void* p, std::size_t) noexcept {
  ::operator delete(p);
}

void operator delete[](void* p, std::size_t) noexcept {
  ::operator delete(p);
}

static void Say(const std::string& text) {
  std::fputs(text.c_str(), stdout);
  std::fputc('\n', stdout);
}

static std::string Num(double value) {
  char buf[32];
  std::snprintf(buf, sizeof buf, "%.1f", value);
  return std::string(buf);
}

class Counted {
 public:
  explicit Counted(const std::string& name) : name_(name) {}
  ~Counted() { Say("   [dtor] " + name_); }

  void OnReading(double value) {
    ++hits_;
    Say("   [" + name_ + "] got " + Num(value) + " (hits=" + std::to_string(hits_) + ")");
  }

  int hits() const { return hits_; }

 private:
  std::string name_;
  int hits_ = 0;
};

// The control group of section B: a plain object, never tracked.
class Plain {
 public:
  ~Plain() { Say("   [Plain dtor]"); }
};

// A slot whose target is a raw pointer, tracked by whoever connected it.
static s2::slot<void(double)> RawSlot(Counted* raw) {
  return s2::slot<void(double)>([raw](double value) { raw->OnReading(value); });
}

int main() {
  setvbuf(stdout, nullptr, _IOLBF, 0);

  {
    Say("=== A1. one tracked receiver ===");
    s2::signal<void(double)> sig;
    boost::shared_ptr<Counted> receiver(new Counted("A1"));
    s2::slot<void(double)> slot = RawSlot(receiver.get());
    slot.track(boost::weak_ptr<void>(receiver));
    sig.connect(slot);
    Say("   num_slots() = " + std::to_string(sig.num_slots()));
    sig(1.0);
    Say("   releasing the last shared_ptr ...");
    receiver.reset();
    Say("   num_slots() = " + std::to_string(sig.num_slots())
        + "  <- counted; the walk pruned the entry");
    sig(2.0);
    Say("   num_slots() = " + std::to_string(sig.num_slots()) + "  <- stays 0");
  }

  {
    Say("");
    Say("=== A2. one slot tracking TWO receivers ===");
    s2::signal<void(double)> sig;
    boost::shared_ptr<Counted> keep(new Counted("kept-alive"));
    boost::shared_ptr<Counted> gone(new Counted("about-to-die"));
    s2::slot<void(double)> slot = RawSlot(keep.get());
    slot.track(boost::weak_ptr<void>(keep));
    slot.track(boost::weak_ptr<void>(gone));
    sig.connect(slot);
    Say("   num_slots() = " + std::to_string(sig.num_slots()));
    sig(1.0);
    Say("   releasing only the second shared_ptr ...");
    gone.reset();
    Say("   num_slots() = " + std::to_string(sig.num_slots())
        + "  <- the first receiver is still alive");
    sig(2.0);
    Say("   kept-alive hits = " + std::to_string(keep->hits())
        + "  <- it stopped too");
  }

  {
    Say("");
    Say("=== B. control group: no track() at all ===");
    s2::signal<void(double)> sig;
    Plain* receiver = new Plain();
    sig.connect([receiver](double) {
      (void)receiver;
      Say("   [untracked slot] ran, with a dangling pointer in the capture");
    });
    Say("   num_slots() = " + std::to_string(sig.num_slots()));
    delete receiver;
    Say("   after delete: num_slots() = " + std::to_string(sig.num_slots())
        + "  <- the library cannot know");
    sig(1.0);
    Say("   <- the slot ran on a dead receiver");
  }

  {
    Say("");
    Say("=== C1. a slot disconnects a sibling, mid-walk ===");
    s2::signal<void(double)> sig;
    Counted first("first"), second("second"), third("third");
    s2::connection c_second;
    s2::connection c_third;
    int first_calls = 0;
    sig.connect([&c_second, &first_calls](double) {
      ++first_calls;
      Say("   [first] disconnecting [second]");
      c_second.disconnect();
    });
    c_second = sig.connect(RawSlot(&second));
    c_third = sig.connect(RawSlot(&third));
    Say("   num_slots() = " + std::to_string(sig.num_slots()));
    sig(1.0);
    Say("   after the round: first=" + std::to_string(first_calls)
        + " second=" + std::to_string(second.hits())
        + " third=" + std::to_string(third.hits()));
  }

  {
    Say("");
    Say("=== C2. a slot disconnects itself, mid-walk ===");
    s2::signal<void(double)> sig;
    Counted other("other");
    s2::connection c_self;
    int self_calls = 0;
    c_self = sig.connect([&c_self, &self_calls](double) {
      ++self_calls;
      Say("   [self] disconnecting itself");
      c_self.disconnect();
    });
    sig.connect(RawSlot(&other));
    sig(1.0);
    Say("   after round 1: self=" + std::to_string(self_calls)
        + " other=" + std::to_string(other.hits()));
    sig(2.0);
    Say("   after round 2: self=" + std::to_string(self_calls)
        + " other=" + std::to_string(other.hits()));
  }

  {
    Say("");
    Say("=== D1. scoped_connection: the IOU expires with the scope ===");
    s2::signal<void(double)> sig;
    Counted receiver("D1");
    {
      s2::scoped_connection sc = sig.connect(RawSlot(&receiver));
      Say("   inside the scope: num_slots() = " + std::to_string(sig.num_slots()));
      sig(1.0);
    }
    Say("   after the scope: num_slots() = " + std::to_string(sig.num_slots()));
    sig(2.0);
    Say("   hits = " + std::to_string(receiver.hits()));
  }

  {
    Say("");
    Say("=== D2. shared_connection_block: stop it without touching the list ===");
    s2::signal<void(double)> sig;
    Counted receiver("D2");
    s2::connection c = sig.connect(RawSlot(&receiver));
    Say("   num_slots() = " + std::to_string(sig.num_slots()));
    {
      s2::shared_connection_block block(c);
      Say("   while blocked: num_slots() = " + std::to_string(sig.num_slots())
          + "  <- the list is untouched");
      sig(1.0);
      Say("   <- nothing ran");
    }
    sig(2.0);
    Say("   hits = " + std::to_string(receiver.hits()));
  }

  {
    Say("");
    Say("=== E. the price of one connection ===");
    s2::signal<void(double)> sig;
    Counted receiver("E");
    long a0 = g_alloc;
    s2::connection c1 = sig.connect(RawSlot(&receiver));
    long a1 = g_alloc;
    s2::connection c2 = sig.connect(RawSlot(&receiver));
    long a2 = g_alloc;
    Say("   connect() #1 (cold): heap allocations = " + std::to_string(a1 - a0));
    Say("   connect() #2 (warm): heap allocations = " + std::to_string(a2 - a1));
    long f0 = g_free;
    c2.disconnect();
    Say("   disconnect():        heap frees = " + std::to_string(g_free - f0)
        + "   allocations = " + std::to_string(g_alloc - a2));
    Say("   totals so far: allocated=" + std::to_string(g_alloc)
        + " freed=" + std::to_string(g_free));
    c1.disconnect();
  }

  return 0;
}
