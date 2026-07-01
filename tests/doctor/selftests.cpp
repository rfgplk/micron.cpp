#include "../../src/io/console.hpp"
#include "../../src/std.hpp"

int
main(void)
{
  if ( !abc::doctor::__selftest_crash_safe() ) {
    mc::console("FAIL: crash-safe handler did not catch a guarded fault");
    return 1;
  }
  for ( int i = 0; i < 16; ++i )
    if ( !abc::doctor::__selftest_crash_safe() ) {
      mc::console("FAIL: crash-safe handler did not recover on repeat (SA_NODEFER?)");
      return 1;
    }

  byte *p = abc::alloc(64);
  if ( !abc::doctor::__selftest_ledger_offheap() ) {
    mc::console("FAIL: ledger is inside the abcmalloc VA (H2 violated)");
    return 2;
  }
  if ( p ) abc::dealloc(p);

  mc::console("PASS: doctor self-tests (crash-safe recovery x17, ledger off-heap)");
  return 0;
}
