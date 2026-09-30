// The same requirement, spelled two ways. The CHECK is identical -- what differs is the
// diagnostic the compiler produces for the same bad call.
//
//   SPELLING=1 -> SFINAE, valid in C++17: a defaulted template argument that has no type
//   SPELLING=2 -> concepts, C++20: a requires-clause on the template
//
// Both reject Take(42). run_language.sh counts how many lines the compiler spends saying so,
// and where it puts the first error -- that count is the entire argument for concepts here.
//
// Build: g++ -std=c++17 -DSPELLING=1 sfinae_vs_concept.cpp   (expected to fail)
//        g++ -std=c++20 -DSPELLING=2 sfinae_vs_concept.cpp   (expected to fail)

#include <cstdio>
#include <type_traits>

#if SPELLING == 2

#include <concepts>

template <class F>
  requires std::invocable<F&, double>
void Take(F&&) {
  std::puts("accepted");
}

#elif SPELLING == 1

template <class F, class = std::enable_if_t<std::is_invocable_v<F&, double>>>
void Take(F&&) {
  std::puts("accepted");
}

#else
#error "define SPELLING to 1 or 2"
#endif

int main() {
  Take(42);  // an int is not callable with a double
  return 0;
}
