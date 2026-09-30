// How much does type erasure cost per receiver?
//
// Counting operator new is the only honest way to answer: std::function may keep the callable
// inline (small-object optimisation) or place it on the heap, and the cut-off is an
// implementation detail of the standard library, not a rule of the language. So measure it.
//
// E(capture size) is swept until the allocation appears -- that boundary is the number worth
// remembering, and it is 16 bytes on libstdc++ (measured, not quoted).
//
// Build: g++ -std=c++17 -Wall -Wextra allocation_cost.cpp -o /tmp/alloc && /tmp/alloc

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <new>
#include <utility>

static int g_allocs = 0;

void* operator new(std::size_t size) {
  ++g_allocs;
  if (void* p = std::malloc(size)) return p;
  throw std::bad_alloc();
}

void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

namespace {

template <class F>
void Measure(const char* label, F sink) {
  g_allocs = 0;
  {
    std::function<void(double)> fn(std::move(sink));
    std::printf("%-24s callable=%2zu bytes  allocations=%d\n", label, sizeof(F), g_allocs);
    (void)fn;
  }
}

}  // namespace

int main() {
  std::printf("sizeof(std::function<void(double)>) = %zu bytes\n\n",
              sizeof(std::function<void(double)>));

  Measure("capture 0 bytes", [](double) {});
  Measure("capture 8 bytes", [a = 0.0](double) { (void)a; });
  Measure("capture 12 bytes", [a = 0, b = 0, c = 0](double) { (void)(a + b + c); });
  Measure("capture 16 bytes", [a = 0.0, b = 0.0](double) { (void)(a + b); });
  Measure("capture 20 bytes", [a = 0, b = 0, c = 0, d = 0, e = 0](double) { (void)(a + b + c + d + e); });
  Measure("capture 24 bytes", [a = 0.0, b = 0.0, c = 0.0](double) { (void)(a + b + c); });
  Measure("capture 32 bytes", [a = 0.0, b = 0.0, c = 0.0, d = 0.0](double) { (void)(a + b + c + d); });
  return 0;
}
