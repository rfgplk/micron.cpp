//  Copyright (c) 2026 David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#ifndef MICRON_WASM_FUZZ_SEED
#define MICRON_WASM_FUZZ_SEED 0x510e527fu
#endif

#include "../../src/bits/eh/cxa_eh_globals.hpp"
#include "../../src/bits/wasm/eh_std.hpp"
#include "fuzz_support.hpp"

#if !defined(__micron_wasm_eh)
#error "fuzz_eh requires the native Wasm EH profile"
#endif

namespace
{
bool
clear_state()
{
  const auto *globals = __cxa_get_globals();
  return WASM_FUZZ_CHECK(globals->uncaughtExceptions == 0) && WASM_FUZZ_CHECK(globals->caughtExceptions == nullptr);
}

bool
caught_state()
{
  const auto *globals = __cxa_get_globals();
  return WASM_FUZZ_CHECK(globals->uncaughtExceptions == 0) && WASM_FUZZ_CHECK(globals->caughtExceptions != nullptr);
}

struct trace {
  u32 value, choices;
  unsigned scopes = 0, constructed = 0, destroyed = 0, nested_constructed = 0, nested_destroyed = 0, handlers = 0;
  const void *object = nullptr;
  bool valid = true;

  void
  check(bool condition)
  {
    valid = condition && valid;
  }
};

struct nested_exception {
  trace &state;
  u32 value;

  nested_exception(trace &s, u32 v) : state(s), value(v) { ++state.nested_constructed; }

  nested_exception(const nested_exception &) = delete;

  ~nested_exception() { ++state.nested_destroyed; }
};

void
nested_catch(trace &state, unsigned level)
{
  void *outer = __cxa_get_globals()->caughtExceptions;
  try {
    throw nested_exception(state, state.value ^ (0x9e3779b9u + level));
  } catch ( const nested_exception &exception ) {
    state.check(caught_state());
    state.check(WASM_FUZZ_CHECK(exception.value == (state.value ^ (0x9e3779b9u + level))));
    state.check(WASM_FUZZ_CHECK(__cxa_get_globals()->caughtExceptions != outer));
  } catch ( ... ) {
    state.check(WASM_FUZZ_CHECK(false));
  }
  state.check(WASM_FUZZ_CHECK(__cxa_get_globals()->caughtExceptions == outer));
  state.check(WASM_FUZZ_CHECK(__cxa_get_globals()->uncaughtExceptions == 0));
}

struct payload {
  trace &state;
  u32 value;

  explicit payload(trace &s) : state(s), value(s.value) { ++state.constructed; }

  payload(const payload &) = delete;

  ~payload()
  {
    state.check(WASM_FUZZ_CHECK(__cxa_get_globals()->uncaughtExceptions == 0));
    ++state.destroyed;
    if ( state.choices & 0x10000u ) nested_catch(state, 31);
  }
};

struct unwind_scope {
  trace &state;
  unsigned ordinal;

  ~unwind_scope()
  {
    // The expected order is 0,1,...,2*depth, independently of catch choices.
    state.check(WASM_FUZZ_CHECK(ordinal == state.scopes));
    state.check(WASM_FUZZ_CHECK(__cxa_get_globals()->uncaughtExceptions == 1));
    ++state.scopes;
  }
};

void
inspect_payload(trace &state, const payload &exception, unsigned level)
{
  state.check(caught_state());
  state.check(WASM_FUZZ_CHECK(exception.value == state.value && &exception.state == &state));
  state.check(WASM_FUZZ_CHECK(state.constructed == 1 && state.destroyed == 0));
  if ( !state.object ) state.object = &exception;
  state.check(WASM_FUZZ_CHECK(state.object == &exception));
  ++state.handlers;
  if ( (state.choices >> level) & 1u ) nested_catch(state, level);
  if ( (state.choices >> (level + 8)) & 1u ) {
    void *outer = __cxa_get_globals()->caughtExceptions;
    try {
      throw;
    } catch ( const payload &again ) {
      state.check(caught_state());
      state.check(WASM_FUZZ_CHECK(&again == &exception));
      state.check(WASM_FUZZ_CHECK(__cxa_get_globals()->caughtExceptions == outer));
    } catch ( ... ) {
      state.check(WASM_FUZZ_CHECK(false));
    }
    state.check(WASM_FUZZ_CHECK(__cxa_get_globals()->caughtExceptions == outer));
  }
}

[[gnu::noinline]] void
propagate(trace &state, unsigned level)
{
  unwind_scope frame{ state, 2 * level };
  if ( !level ) throw payload(state);
  try {
    propagate(state, level - 1);
  } catch ( const payload &exception ) {
    inspect_payload(state, exception, level);
    unwind_scope handler{ state, 2 * level - 1 };
    throw;
  }
}

bool
lifetimes(u32 value, u32 choices, unsigned depth)
{
  trace state{ value, choices };
  try {
    propagate(state, depth);
    return WASM_FUZZ_CHECK(false);
  } catch ( const payload &exception ) {
    inspect_payload(state, exception, depth + 1);
    state.check(WASM_FUZZ_CHECK(state.scopes == 2 * depth + 1));
  } catch ( ... ) {
    return WASM_FUZZ_CHECK(false);
  }
  unsigned expected_nested = bool(choices & 0x10000u);
  for ( unsigned level = 1; level <= depth + 1; ++level ) expected_nested += (choices >> level) & 1u;
  return WASM_FUZZ_CHECK(state.valid) && WASM_FUZZ_CHECK(state.constructed == 1 && state.destroyed == 1)
         && WASM_FUZZ_CHECK(state.handlers == depth + 1) && WASM_FUZZ_CHECK(state.nested_constructed == expected_nested)
         && WASM_FUZZ_CHECK(state.nested_destroyed == expected_nested) && clear_state();
}

struct construction_counts {
  unsigned member_constructed = 0, member_destroyed = 0, attempts = 0, complete_destroyed = 0;
  bool valid = true;
};

struct member {
  construction_counts &counts;

  explicit member(construction_counts &c) : counts(c) { ++counts.member_constructed; }

  ~member()
  {
    ++counts.member_destroyed;
    counts.valid = WASM_FUZZ_CHECK(__cxa_get_globals()->uncaughtExceptions == 1) && counts.valid;
  }
};

struct failed_construction {
  member subobject;
  construction_counts &counts;

  failed_construction(construction_counts &c, u32 value) : subobject(c), counts(c)
  {
    ++counts.attempts;
    throw value;
  }

  ~failed_construction() { ++counts.complete_destroyed; }
};

bool
failed_throw(u32 value)
{
  construction_counts counts;
  try {
    throw failed_construction(counts, value);
  } catch ( u32 actual ) {
    if ( !WASM_FUZZ_CHECK(actual == value) || !caught_state() ) return false;
  } catch ( ... ) {
    return WASM_FUZZ_CHECK(false);
  }
  return WASM_FUZZ_CHECK(counts.valid) && WASM_FUZZ_CHECK(counts.member_constructed == 1 && counts.member_destroyed == 1)
         && WASM_FUZZ_CHECK(counts.attempts == 1 && counts.complete_destroyed == 0) && clear_state();
}

struct destruction_counts {
  unsigned roots = 0, leaves = 0, pads = 0;
};

struct root {
  destruction_counts *counts;
  u32 value;

  explicit root(destruction_counts *c = nullptr, u32 v = 0) : counts(c), value(v) { }

  virtual ~root()
  {
    if ( counts ) ++counts->roots;
  }

  u32
  read() const noexcept
  {
    return value;
  }

  void
  touch() noexcept
  {
    ++value;
  }
};

struct pad {
  destruction_counts *counts;

  explicit pad(destruction_counts *c = nullptr) : counts(c) { }

  virtual ~pad()
  {
    if ( counts ) ++counts->pads;
  }
};

struct public_object: pad, root {
  explicit public_object(destruction_counts *c = nullptr, u32 v = 0) : pad(c), root(c, v) { }

  ~public_object()
  {
    if ( root::counts ) ++root::counts->leaves;
  }
};

struct private_object: private root {
  explicit private_object(destruction_counts *c = nullptr, u32 v = 0) : root(c, v) { }

  u32
  token() const
  {
    return value;
  }

  ~private_object()
  {
    if ( counts ) ++counts->leaves;
  }
};

struct left_root: root {
  using root::root;
};

struct right_root: root {
  using root::root;
};

struct ambiguous_object: left_root, right_root {
  explicit ambiguous_object(destruction_counts *c = nullptr, u32 v = 0) : left_root(c, v), right_root(c, ~v) { }

  ~ambiguous_object()
  {
    if ( left_root::counts ) ++left_root::counts->leaves;
  }
};

struct virtual_left: virtual root {
};

struct virtual_right: virtual root {
};

struct diamond: virtual_left, virtual_right {
  explicit diamond(destruction_counts *c = nullptr, u32 v = 0) : root(c, v) { }

  ~diamond()
  {
    if ( counts ) ++counts->leaves;
  }
};

struct middle: virtual root {
};

struct branch_left: middle {
};

struct branch_right: middle {
};

struct split_middle: branch_left, branch_right {
};

struct public_side {
  virtual ~public_side() { }
};

struct private_side {
  virtual ~private_side() { }
};

struct mixed_access: public public_side, private private_side {
};

bool
class_matching(unsigned shape, u32 value)
{
  destruction_counts counts;
  bool valid = false;
  switch ( shape ) {
  case 0:
    try {
      throw public_object(&counts, value);
    } catch ( const root &object ) {
      const auto *complete = dynamic_cast<const public_object *>(&object);
      valid = WASM_FUZZ_CHECK(object.value == value && complete != nullptr)
              && WASM_FUZZ_CHECK(static_cast<const root *>(complete) == &object)
              && WASM_FUZZ_CHECK(dynamic_cast<const pad *>(&object) == static_cast<const pad *>(complete)) && caught_state();
    } catch ( ... ) {
    }
    break;
  case 1:
    try {
      throw private_object(&counts, value);
    } catch ( const root & ) {
    } catch ( const private_object &object ) {
      valid = WASM_FUZZ_CHECK(object.token() == value) && caught_state();
    } catch ( ... ) {
    }
    break;
  case 2:
    try {
      throw ambiguous_object(&counts, value);
    } catch ( const root & ) {
    } catch ( const ambiguous_object &object ) {
      valid = WASM_FUZZ_CHECK(object.left_root::value == value && object.right_root::value == ~value) && caught_state();
    } catch ( ... ) {
    }
    break;
  default:
    try {
      throw diamond(&counts, value);
    } catch ( const root &object ) {
      const auto *complete = dynamic_cast<const diamond *>(&object);
      valid = WASM_FUZZ_CHECK(object.value == value && complete != nullptr)
              && WASM_FUZZ_CHECK(static_cast<const root *>(static_cast<const virtual_left *>(complete)) == &object)
              && WASM_FUZZ_CHECK(static_cast<const root *>(static_cast<const virtual_right *>(complete)) == &object) && caught_state();
    } catch ( ... ) {
    }
    break;
  }
  // Counts derive from the four declared layouts, not the runtime's RTTI graph.
  return WASM_FUZZ_CHECK(valid) && WASM_FUZZ_CHECK(counts.roots == (shape == 2 ? 2u : 1u)) && WASM_FUZZ_CHECK(counts.leaves == 1)
         && WASM_FUZZ_CHECK(counts.pads == (shape == 0 ? 1u : 0u)) && clear_state();
}

template<typename Caught, typename Thrown>
bool
pointer_matching(Thrown thrown, Caught expected, bool wanted = true)
{
  bool matched = false, valid = true;
  try {
    throw thrown;
  } catch ( Caught actual ) {
    matched = true;
    valid = WASM_FUZZ_CHECK(actual == expected) && caught_state();
  } catch ( ... ) {
    valid = caught_state();
  }
  return WASM_FUZZ_CHECK(matched == wanted) && WASM_FUZZ_CHECK(valid) && clear_state();
}

void
function()
{
}

void
nothrow_function() noexcept
{
}

bool
pointer_cases(unsigned choice, bool null, u32 value)
{
  int integer = int(value & 0x7fffffffu);
  int *plain = null ? nullptr : &integer;
  const int *constant = plain;
  volatile int *changing = plain;
  const volatile int *qualified = plain;
  int **indirect = &plain;
  public_object object(nullptr, value);
  diamond virtual_object(nullptr, value);
  switch ( choice ) {
  case 0:
    return pointer_matching<const int *>(plain, plain);
  case 1:
    return pointer_matching<volatile int *>(plain, plain);
  case 2:
    return pointer_matching<const volatile int *>(plain, plain);
  case 3:
    return pointer_matching<int *>(constant, nullptr, false);
  case 4:
    return pointer_matching<const int *>(changing, nullptr, false);
  case 5:
    return pointer_matching<volatile int *>(qualified, nullptr, false);
  case 6:
    return pointer_matching<const int **>(indirect, nullptr, false);
  case 7:
    return pointer_matching<const int *const *>(indirect, indirect);
  case 8:
    return pointer_matching<void *>(plain, plain);
  case 9:
    return pointer_matching<const void *>(constant, constant);
  case 10:
    return pointer_matching<void *>(constant, nullptr, false);
  case 11:
    return pointer_matching<void (*)()>(nothrow_function, nothrow_function);
  case 12:
    return pointer_matching<void (*)() noexcept>(function, nullptr, false);
  case 13:
    return pointer_matching<root *>(nullptr, nullptr);
  case 14:
    return pointer_matching<u32 root::*>(nullptr, nullptr);
  case 15:
    return pointer_matching<void (root::*)()>(nullptr, nullptr);
  case 16: {
    u32 root::*pointer = null ? nullptr : &root::value;
    return pointer_matching<u32 root::*>(pointer, pointer) && (null || WASM_FUZZ_CHECK(object.*pointer == value));
  }
  case 17: {
    u32 (root::*pointer)() const noexcept = null ? nullptr : &root::read;
    return pointer_matching<u32 (root::*)() const>(pointer, pointer) && (null || WASM_FUZZ_CHECK((object.*pointer)() == value));
  }
  case 18:
    return pointer_matching<root *>(null ? nullptr : &object, null ? nullptr : static_cast<root *>(&object));
  case 19:
    return pointer_matching<root *>(static_cast<private_object *>(nullptr), nullptr, false);
  case 20:
    return pointer_matching<root *>(static_cast<ambiguous_object *>(nullptr), nullptr, false);
  case 21:
    return pointer_matching<root *>(null ? nullptr : &virtual_object, null ? nullptr : static_cast<root *>(&virtual_object));
  case 22:
    return pointer_matching<void *>(static_cast<int *>(nullptr), nullptr);
  default:
    return pointer_matching<void *>(function, nullptr, false);
  }
}

[[gnu::noinline]] bool
dynamic_cases(unsigned choice, u32 value)
{
  public_object object(nullptr, value);
  root unrelated(nullptr, ~value);
  ambiguous_object ambiguous(nullptr, value);
  diamond virtual_object(nullptr, value);
  split_middle repeated;
  mixed_access hidden;
  root *sources[]{ &object, &unrelated, static_cast<left_root *>(&ambiguous), &virtual_object, &repeated, nullptr };
  root *source = sources[choice % 6];
  if ( !WASM_FUZZ_CHECK(dynamic_cast<void *>(source)
                        == (choice % 6 == 0   ? static_cast<void *>(&object)
                            : choice % 6 == 1 ? static_cast<void *>(&unrelated)
                            : choice % 6 == 2 ? static_cast<void *>(&ambiguous)
                            : choice % 6 == 3 ? static_cast<void *>(&virtual_object)
                            : choice % 6 == 4 ? static_cast<void *>(&repeated)
                                              : nullptr)) )
    return false;
  switch ( choice % 6 ) {
  case 0:
    if ( !WASM_FUZZ_CHECK(dynamic_cast<public_object *>(source) == &object)
         || !WASM_FUZZ_CHECK(dynamic_cast<pad *>(source) == static_cast<pad *>(&object))
         || !WASM_FUZZ_CHECK(&typeid(*source) == &typeid(public_object)) )
      return false;
    break;
  case 1:
    if ( !WASM_FUZZ_CHECK(dynamic_cast<public_object *>(source) == nullptr) ) return false;
    try {
      (void)dynamic_cast<public_object &>(*source);
      return WASM_FUZZ_CHECK(false);
    } catch ( const std::bad_cast & ) {
      if ( !caught_state() ) return false;
    } catch ( ... ) {
      return WASM_FUZZ_CHECK(false);
    }
    break;
  case 2:
    if ( !WASM_FUZZ_CHECK(dynamic_cast<ambiguous_object *>(source) == &ambiguous)
         || !WASM_FUZZ_CHECK(dynamic_cast<right_root *>(source) == static_cast<right_root *>(&ambiguous)) )
      return false;
    break;
  case 3:
    if ( !WASM_FUZZ_CHECK(dynamic_cast<diamond *>(source) == &virtual_object)
         || !WASM_FUZZ_CHECK(dynamic_cast<virtual_left *>(source) == static_cast<virtual_left *>(&virtual_object))
         || !WASM_FUZZ_CHECK(dynamic_cast<virtual_right *>(source) == static_cast<virtual_right *>(&virtual_object)) )
      return false;
    break;
  case 4:
    if ( !WASM_FUZZ_CHECK(dynamic_cast<middle *>(source) == nullptr)
         || !WASM_FUZZ_CHECK(dynamic_cast<branch_left *>(source) == static_cast<branch_left *>(&repeated))
         || !WASM_FUZZ_CHECK(dynamic_cast<branch_right *>(source) == static_cast<branch_right *>(&repeated)) )
      return false;
    break;
  default:
    if ( !WASM_FUZZ_CHECK(dynamic_cast<public_object *>(source) == nullptr) ) return false;
    try {
      (void)typeid(*source);
      return WASM_FUZZ_CHECK(false);
    } catch ( const std::bad_typeid & ) {
      if ( !caught_state() ) return false;
    } catch ( ... ) {
      return WASM_FUZZ_CHECK(false);
    }
    break;
  }
  public_side *public_source = &hidden;
  return WASM_FUZZ_CHECK(dynamic_cast<private_side *>(public_source) == nullptr)
         && WASM_FUZZ_CHECK(dynamic_cast<mixed_access *>(public_source) == &hidden) && clear_state();
}
}      // namespace

int
main()
{
  if ( !clear_state() ) return 6;
  for ( unsigned shape = 0; shape < 4; ++shape )
    if ( !class_matching(shape, 0x7a4d9123u + shape) ) return 6;
  for ( unsigned choice = 0; choice < 24; ++choice )
    for ( unsigned null = 0; null < 2; ++null )
      if ( !pointer_cases(choice, null, 0xc35a1907u) ) return 6;
  for ( unsigned choice = 0; choice < 6; ++choice )
    if ( !dynamic_cases(choice, 0x6b2918e5u) ) return 6;

  u32 random = wasm_fuzz::seed;
  for ( u32 iteration = 0; iteration < wasm_fuzz::steps; ++iteration ) {
    wasm_fuzz::step = iteration;
    const u32 value = wasm_fuzz::next(random), choices = wasm_fuzz::next(random);
    const unsigned depth = wasm_fuzz::next(random) % 8;
    if ( !clear_state() || !lifetimes(value, choices, depth) || !failed_throw(value ^ choices)
         || !class_matching(wasm_fuzz::next(random) % 4, value) || !pointer_cases(wasm_fuzz::next(random) % 24, choices & 1u, value)
         || !dynamic_cases(wasm_fuzz::next(random) % 6, value) || !clear_state() )
      return 6;
  }
  return wasm_fuzz::finish("eh");
}
