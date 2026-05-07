// vector_members.cpp
// Comprehensive coverage of micron::vector<T> member functions.
// Covers: insert, erase, pop variants, resize/reserve, fill, swap,
// slice operations, clone, weld/append, find.
//
// See also:
//   examples/vector.cpp — quick tour
//   examples/svector.cpp — stack-allocated bounded variant
//   examples/io.cpp — println(vector) container printing
//
// IMPORTANT deviations from std::vector:
//   - max_size()   = allocated capacity   (std:: uses capacity())
//   - operator+=   = push_back            (no std:: equivalent)
//   - weld(v)      = move-append all of v (no std:: equivalent)
//   - find()       = returns iterator     (no std:: member find)
//   - operator[](from,to) = slice         (no std:: equivalent)
//   - set_size(n)  = forcibly set length without construct/destruct
//                    (dangerous; only when storage is initialised)

#include "../src/except.hpp"
#include "../src/io/console.hpp"
#include "../src/vector/vector.hpp"

struct Tracked {
  static inline int alive = 0;
  int val;
  explicit Tracked(int v = 0) : val(v) { ++alive; }
  Tracked(const Tracked &o) : val(o.val) { ++alive; }
  Tracked(Tracked &&o) noexcept : val(o.val)
  {
    o.val = -1;
    ++alive;
  }
  ~Tracked() { --alive; }
  Tracked &
  operator=(const Tracked &o)
  {
    val = o.val;
    return *this;
  }
  Tracked &
  operator=(Tracked &&o) noexcept
  {
    val = o.val;
    o.val = -1;
    return *this;
  }
  bool
  operator==(const Tracked &o) const
  {
    return val == o.val;
  }
};

int
main()
{
  // ================================================================
  // CONSTRUCTION
  // ================================================================
  micron::io::println("-- construction --");

  micron::vector<int> v1(5, 7);     // 5 elements, each = 7
  micron::io::println("size+value: ", v1);

  int n = 0;
  micron::vector<int> v2(6, [&n]() {
    int v = n * n;
    n++;
    return v;
  });
  micron::io::println("generator : ", v2);

  micron::vector<int> v3({100, 200, 300});
  micron::io::println("init-list : ", v3);

  // ================================================================
  // PUSH / POP
  // ================================================================
  micron::io::println("-- push/pop --");

  micron::vector<int> v({1, 2, 3});
  v.push_back(4);
  v.push_back(5);
  micron::io::println("after pushes: ", v);

  v.pop_back();
  micron::io::println("after pop_back: ", v);

  v += 10;     // operator+= is push_back — sugar
  v += 20;
  micron::io::println("after += sugar: ", v);

  micron::vector<Tracked> tracked;
  tracked.emplace_back(42);
  tracked.emplace_back(99);
  micron::io::println("emplace_back: alive=", Tracked::alive, " [0].val=", tracked[0].val);

  // ================================================================
  // INSERT
  // ================================================================
  micron::io::println("-- insert --");

  micron::vector<int> ins({1, 2, 4, 5});
  ins.insert(usize(2), 3);     // insert 3 at index 2
  micron::io::println("insert(2,3)        : ", ins);

  ins.insert(usize(0), 0);
  micron::io::println("insert(0,0)        : ", ins);

  ins.insert(usize(2), 99, usize(3));     // insert 3 copies of 99 at index 2
  micron::io::println("insert(2,99,3-copies): ", ins);

  // ================================================================
  // ERASE
  // ================================================================
  micron::io::println("-- erase --");

  micron::vector<int> er({10, 20, 30, 40, 50});
  er.erase(usize(1));
  micron::io::println("erase(1)   : ", er);

  er.erase(usize(0), usize(2));     // erase 2 elements starting at 0
  micron::io::println("erase(0,2) : ", er);

  // ================================================================
  // RESIZE / RESERVE
  // ================================================================
  micron::io::println("-- resize/reserve --");

  micron::vector<int> rv;
  rv.reserve(64);
  micron::io::println("reserve(64) max_size=", rv.max_size());

  rv.resize(8, 42);
  micron::io::println("resize(8, 42) : ", rv);

  rv.resize(4);
  micron::io::println("resize(4)     : ", rv);
  // shrink_to_fit is not implemented in micron::vector. To reclaim
  // memory, do: auto shrunk = rv.clone(); rv = micron::move(shrunk);

  // ================================================================
  // FILL — overwrite all elements with a value
  // ================================================================
  micron::io::println("-- fill --");

  micron::vector<int> fl(5, 0);
  fl.fill(77);
  micron::io::println("fill(77) : ", fl);

  // ================================================================
  // SWAP
  // ================================================================
  micron::io::println("-- swap --");

  micron::vector<int> sa({1, 2, 3});
  micron::vector<int> sb({4, 5, 6, 7, 8});
  sa.swap(sb);
  micron::io::println("after swap: sa=", sa, " sb=", sb);

  // ================================================================
  // CLONE — deep copy (independent of source)
  // ================================================================
  micron::io::println("-- clone --");

  micron::vector<int> orig({1, 2, 3, 4, 5});
  auto cloned = orig.clone();
  cloned[0] = 999;
  micron::io::println("orig=  ", orig);
  micron::io::println("cloned=", cloned);

  // ================================================================
  // APPEND (copy) vs WELD (move-append)
  // ================================================================
  micron::io::println("-- append/weld --");

  micron::vector<int> base({1, 2, 3});
  micron::vector<int> extra({4, 5, 6});
  base.append(extra);
  micron::io::println("append: base=", base, " extra=", extra);

  micron::vector<int> to_weld({7, 8, 9});
  base.weld(micron::move(to_weld));
  micron::io::println("weld:   base=", base, " to_weld.empty=", to_weld.empty());

  // ================================================================
  // FIND — returns iterator to first match
  // ================================================================
  micron::io::println("-- find --");

  micron::vector<int> finder({10, 20, 30, 20, 10});
  auto it = finder.find(20);
  micron::io::println("find(20) offset = ", it - finder.begin());
  auto miss = finder.find(99);
  micron::io::println("find(99) not found = ", miss == finder.end());

  // ================================================================
  // SLICING — operator[](from, to) returns a non-owning slice
  // ================================================================
  micron::io::println("-- slicing --");

  micron::vector<int> s({0, 1, 2, 3, 4, 5, 6, 7, 8, 9});
  auto slice = s[2u, 6u];     // elements [2, 6) — indices 2,3,4,5
  micron::io::print("slice [2,6): ");
  for ( auto *p = slice.cbegin(); p != slice.cend(); ++p ) micron::io::print(*p, " ");
  micron::io::println("");

  // ================================================================
  // set_size — DANGEROUS: set length without constructing
  // Use only when storage was initialised by other means (e.g. memcpy).
  // ================================================================
  micron::io::println("-- set_size (raw) --");

  micron::vector<int> manual;
  manual.reserve(8);
  int vals[4] = {10, 20, 30, 40};
  micron::memcpy(manual.data(), vals, u64(4));
  manual.set_size(4);
  micron::io::println("set_size(4): ", manual);

  return 0;
}
