// Can the producer hold something that identifies ONE receiver?
//
// This is the question section 6 ended on: who is responsible for voiding the receipt. Layer 1
// answered it with a pointer, because a pointer can be compared. After type erasure there is
// nothing to compare, so the producer has to hand out its own token.
//
// EXPECTED: compiles for GEN=1, FAILS for GEN=2 and GEN=3.
//
//   GEN=1 -> g++ -std=c++17 -DGEN=1 identity.cpp
//   GEN=2 -> g++ -std=c++17 -DGEN=2 identity.cpp   (no operator== for std::function)

#include <functional>

#if GEN == 1
class Observer {
 public:
  virtual ~Observer() = default;
  virtual void OnReading(double value) = 0;
};

void Probe(Observer* a, Observer* b) {
  const bool same = (a == b);
  (void)same;
}
#else
void Probe(std::function<void(double)> a, std::function<void(double)> b) {
  const bool same = (a == b);  // no such comparison exists
  (void)same;
}
#endif

int main() {
#if GEN == 1
  Observer* a = nullptr;
  Observer* b = nullptr;
  Probe(a, b);
#else
  std::function<void(double)> a = [](double) {};
  std::function<void(double)> b = [](double) {};
  Probe(a, b);
#endif
  return 0;
}
