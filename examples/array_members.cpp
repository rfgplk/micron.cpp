// array_members.cpp
// Comprehensive coverage of micron::array<T,N> member functions.
// Constructors, element access, iteration, array-specific ops not in
// array.cpp.
//
// See also:
//   examples/array.cpp      — quick tour
//   examples/io.cpp         — println(array) container printing
//   examples/algorithm.cpp  — algorithms that act on arrays

#include "../src/algorithm/find.hpp"
#include "../src/array/array.hpp"
#include "../src/except.hpp"
#include "../src/io/console.hpp"

struct Counter {
  static inline int ctor_count = 0;
  static inline int dtor_count = 0;
  int val;
  Counter() : val(0) { ++ctor_count; }
  explicit Counter(int v) : val(v) { ++ctor_count; }
  Counter(const Counter &o) : val(o.val) { ++ctor_count; }
  Counter(Counter &&o) noexcept : val(o.val) { o.val = -1; ++ctor_count; }
  ~Counter() { ++dtor_count; }
  Counter &operator=(const Counter &o) { val = o.val; return *this; }
  Counter &operator=(Counter &&o) noexcept { val = o.val; o.val = -1; return *this; }
  bool operator==(const Counter &o) const { return val == o.val; }
};

int
main()
{
  // ================================================================
  // CONSTRUCTION
  // ================================================================

  // Default: all elements value-initialised (T{} for class, 0 for trivial)
  micron::array<int, 4> def;
  micron::io::println("default       : ", def);

  // Broadcast: single value fills every slot
  micron::array<int, 4> filled(99);
  micron::io::println("broadcast(99) : ", filled);

  // Generator: nullary lambda called once per element
  int seq = 0;
  micron::array<int, 6> gen([&seq]() { return seq++; });
  micron::io::println("generator     : ", gen);

  // Initializer list
  micron::array<int, 4> from_list({10, 20, 30, 40});
  micron::io::println("init-list     : ", from_list);

  // Copy / move
  micron::array<int, 4> copy = from_list;
  copy[0] = 999;
  micron::io::println("copy: ", copy, "  original: ", from_list);

  micron::array<int, 4> moved = micron::move(from_list);
  micron::io::println("moved        : ", moved);

  // Object semantics: constructors and destructors called correctly
  Counter::ctor_count = 0;
  Counter::dtor_count = 0;
  {
    micron::array<Counter, 3> obj_arr;
    micron::io::println("object array ctor_count=", Counter::ctor_count);   // 3
  }
  micron::io::println("after scope dtor_count=", Counter::dtor_count);   // 3

  // ================================================================
  // ELEMENT ACCESS
  // ================================================================

  micron::array<int, 8> a({1, 2, 3, 4, 5, 6, 7, 8});

  // operator[] — unchecked, no bounds validation
  micron::io::println("a[3]=", a[3]);

  // at() — bounds-checked; throws on OOR
  micron::io::println("a.at(7)=", a.at(7));
  try {
    a.at(8);   // N=8, index 8 is OOR
  } catch ( const micron::except::runtime_error &ex ) {
    micron::io::println("at(8) threw: ", ex.what());
  }

  // data() — raw pointer to the first element
  int *raw = a.data();
  micron::io::println("data()[0]=", raw[0]);

  // addr() — returns `this` (array<T,N>* — pointer to the array object itself)
  auto *self = a.addr();
  micron::io::println("addr() is this: ", reinterpret_cast<usize>(self));

  // ================================================================
  // ITERATORS
  // ================================================================

  micron::array<int, 5> b({10, 20, 30, 40, 50});

  // Range-based for (uses begin/end)
  int rfor_sum = 0;
  for ( int x : b ) rfor_sum += x;
  micron::io::println("range-for sum=", rfor_sum);

  // Manual iterator loop
  int manual_sum = 0;
  for ( auto *it = b.cbegin(); it != b.cend(); ++it ) manual_sum += *it;
  micron::io::println("manual iterator sum=", manual_sum);

  // Iterator arithmetic (random access)
  auto *mid = b.begin() + 2;
  micron::io::println("mid iterator *mid=", *mid);

  // ================================================================
  // SIZE AND CAPACITY (compile-time constants)
  // ================================================================

  micron::io::println("b.size()=", b.size());                    // 5
  micron::io::println("b.max_size()=", b.max_size());            // 5
  micron::io::println("static_size=", decltype(b)::static_size); // 5
  micron::io::println("length=", decltype(b)::length);           // 5

  // static_size is accessible without an instance — useful in templates
  static_assert(micron::array<int, 16>::static_size == 16);

  // ================================================================
  // CLEAR
  // ================================================================

  // clear() destroys all elements and re-initializes to T{}
  // For trivial types this just zeroes the storage
  micron::array<int, 4> cl({1, 2, 3, 4});
  cl.clear();
  micron::io::println("after clear: cl[0]=", cl[0]);   // 0

  // ================================================================
  // EQUALITY (via algorithm::equal — array has no operator==)
  // ================================================================

  micron::array<int, 3> x({1, 2, 3});
  micron::array<int, 3> y({1, 2, 3});
  micron::array<int, 3> z({1, 2, 4});
  // Use micron::equal from find.hpp for element-wise comparison
  bool xy_eq = micron::equal(x.cbegin(), x.cend(), y.cbegin());
  bool xz_eq = micron::equal(x.cbegin(), x.cend(), z.cbegin());
  micron::io::println("x==y (via equal): ", xy_eq);
  micron::io::println("x==z (via equal): ", xz_eq);

  // ================================================================
  // ARITHMETIC OPERATORS (element-wise)
  // ================================================================

  micron::array<int, 4> p({1, 2, 3, 4});
  micron::array<int, 4> q({10, 10, 10, 10});

  p += q;
  micron::io::println("p += q: ", p);
  p -= q;
  micron::io::println("p -= q: ", p);
  p *= q;
  micron::io::println("p *= q: ", p);

  // Scalar broadcast form
  micron::array<int, 4> scalar_test({2, 4, 6, 8});
  scalar_test += micron::array<int, 4>(100);     // adds 100 to every element
  micron::io::println("scalar +=100: ", scalar_test);

  return 0;
}
