// pointers.cpp
// micron smart pointer types (src/pointer.hpp, src/memory/pointers/).
//
// See also:
//   examples/memory_alloc.cpp        — chunk<T> + allocator_serial
//   examples/memory_memcpy_cmp.cpp   — micron::memcpy / memcmp / memmove
//   examples/memory_memset.cpp       — micron::memset variants
//
// Aliases (from pointer.hpp):
//   ptr<T>      = unique_pointer<T>   — sole owner, non-copyable
//   sptr<T>     = shared_pointer<T>   — ref-counted shared owner
//   wptr<T>     = weak_pointer<T>     — non-owning observer
//   atom_ptr<T> = atomic_pointer<T>   — thread-safe unique owner
//   cptr<T>     = const_pointer<T>    — immutable value
//
// Key STL deltas:
//   - ptr<T> cannot be constructed from a raw lvalue pointer — you must
//     move the raw pointer in. This prevents two owners of the same
//     address.
//   - ptr<T>(Args...) allocates AND constructs — no make_unique needed.
//   - sptr<T> embeds the control block; no separate heap allocation
//     (unlike std::make_shared).

#include "../src/pointer.hpp"
#include "../src/io/console.hpp"

struct Widget {
  int id;
  explicit Widget(int i) : id(i) { micron::io::println("Widget(", i, ") constructed"); }
  ~Widget() { micron::io::println("Widget(", id, ") destroyed"); }
};

// --- unique ownership (ptr<T>) ---
static void
demo_unique()
{
  micron::io::println("-- unique_pointer --");

  // Construct in-place: allocates and calls Widget(42)
  micron::ptr<Widget> p(42);
  micron::io::println("id=", p->id);

  // Move transfers ownership; source becomes null
  micron::ptr<Widget> q = micron::move(p);
  micron::io::println("after move: p is null=", !p, " q id=", q->id);

  // Copy is deleted — this would not compile:
  // micron::ptr<Widget> r = q;

  // Assign from rvalue raw pointer (source zeroed)
  Widget *raw = new Widget(99);
  micron::ptr<Widget> r(micron::move(raw));   // raw becomes nullptr
  micron::io::println("raw zeroed=", raw == nullptr);
}   // q and r destroyed here

// --- shared ownership (sptr<T>) ---
static void
demo_shared()
{
  micron::io::println("-- shared_pointer --");

  micron::sptr<Widget> a(10);              // allocates Widget(10)
  micron::io::println("refs=", a.refs());

  {
    micron::sptr<Widget> b = a;            // copy increments ref count
    micron::io::println("refs after copy=", a.refs());
    micron::io::println("b->id=", b->id);
  }   // b destroyed, count drops

  micron::io::println("refs after scope=", a.refs());
}   // a destroyed, Widget(10) freed

// --- weak observation (wptr<T>) ---
static void
demo_weak()
{
  micron::io::println("-- weak_pointer --");

  micron::sptr<Widget> owner(20);
  micron::wptr<Widget> obs(owner);         // does not affect ref count

  micron::io::println("obs->id=", obs->id);
  micron::io::println("obs is null=", !obs);
}

// --- atomic pointer (atom_ptr<T>) ---
static void
demo_atomic()
{
  micron::io::println("-- atomic_pointer --");

  micron::atom_ptr<Widget> ap(30);
  micron::io::println("ap->id=", ap->id);

  // get() performs an atomic load (default: memory_order_acquire)
  Widget *raw = ap.get();
  micron::io::println("raw id via get()=", raw->id);

  // exchange(T*&&) — atomically replaces stored pointer with a new raw one,
  // returning the previous pointer (non-owning). atom_ptr no longer owns old.
  Widget *prev = ap.exchange(nullptr);   // returns the Widget*, ap now null
  micron::io::println("after exchange: ap is null=", !ap, " prev id=", prev->id);
  // prev is a raw non-owning pointer — caller must manage its lifetime.
  delete prev;   // manual cleanup since ap relinquished ownership
}

int
main()
{
  demo_unique();
  micron::io::println("");
  demo_shared();
  micron::io::println("");
  demo_weak();
  micron::io::println("");
  demo_atomic();
  return 0;
}
