// exceptions.cpp
// micron's exception hierarchy (src/except.hpp).
//
// See also:
//   examples/control.cpp     — process / signal handling
//   examples/filesystem.cpp  — exceptions thrown by io::cached_file
//
// Key differences from std::
//   - All exceptions inherit from micron::except::__base_exception
//     (NOT std::exception).
//   - Each carries a numeric error code via which(), offset from 0x7f00.
//   - Short aliases live directly in micron:: (micron::oor,
//     micron::runtime, ...).
//   - No default what() string — pass a message to the constructor.

#include "../src/except.hpp"
#include "../src/io/console.hpp"

// --- Function that throws a micron exception ---
static int
safe_div(int a, int b)
{
  if ( b == 0 )
    throw micron::domain("division by zero");   // micron::domain = except::domain_error
  return a / b;
}

// --- Bounds checking with out-of-range ---
static int
get_element(const int *arr, usize sz, usize idx)
{
  if ( idx >= sz )
    throw micron::oor("index out of bounds");   // micron::oor = except::out_of_range_error
  return arr[idx];
}

// --- Catching by base class ---
static void
demo_base_catch()
{
  try {
    throw micron::runtime("something went wrong at runtime");
  } catch ( const micron::except::__base_exception &ex ) {
    micron::io::println("caught via base: ", ex.what());
  }
}

// --- Error codes ---
static void
demo_error_codes()
{
  // Each exception type has a unique code accessible via which().
  // Codes are offset from 0x7f00 so they don't collide with errno values.
  micron::except::memory_error mem_ex("allocation failed");
  micron::except::network_error net_ex("connection refused");

  micron::io::println("memory_error code: ", mem_ex.which());   // 0x7f11
  micron::io::println("network_error code: ", net_ex.which()); // 0x7f14
}

// --- Micron-specific (non-standard) exception types ---
// These have no std:: equivalents:
//   micron::library    — bug in micron internals
//   micron::hardware   — hardware fault (SIGFPE etc.)
//   micron::memory_err — allocation failure
//   micron::critical_err — unrecoverable, code 0x7fff
//   micron::thread_err — threading error
//   micron::fs_error   — filesystem error
//   micron::net_error  — network error

int
main()
{
  // Basic throw/catch
  try {
    int r = safe_div(10, 0);
    (void)r;
  } catch ( const micron::domain &ex ) {
    micron::io::println("domain caught: ", ex.what(), " code=", ex.which());
  }

  // Out of range
  int arr[3] = {1, 2, 3};
  try {
    get_element(arr, 3, 99);
  } catch ( const micron::oor &ex ) {
    micron::io::println("oor caught: ", ex.what());
  }

  // Base class catch
  demo_base_catch();

  // Error codes
  demo_error_codes();

  // Multiple exception types in one try block
  try {
    throw micron::io_err("pipe broken");
  } catch ( const micron::domain &) {
    micron::io::println("domain (unreachable)");
  } catch ( const micron::io_err &ex ) {
    micron::io::println("io_err caught: ", ex.what());
  }

  // logic_error for invariant violations
  try {
    throw micron::logic("precondition violated");
  } catch ( const micron::logic &ex ) {
    micron::io::println("logic caught: ", ex.what());
  }

  micron::io::println("all exception examples done");
  return 0;
}
