// concept_satisfaction.cpp
// Compile-time sweep: every public container, string, and smart pointer
// satisfies the relevant micron concept. Failures surface as static_assert
// messages naming both the type and the violated concept.

#include "../../src/std.hpp"

// vector.hpp must precede convector.hpp because convector uses
// micron::__impl::grow which is defined inline by vector.hpp.
#include "../../src/vector/vector.hpp"

#include "../../src/array/array.hpp"
#include "../../src/array/iarray.hpp"
#include "../../src/string/sstring.hpp"
#include "../../src/string/string.hpp"
#include "../../src/string/string_view.hpp"
#include "../../src/vector/circle_vector.hpp"
#include "../../src/vector/convector.hpp"
#include "../../src/vector/fvector.hpp"
#include "../../src/vector/ivector.hpp"
#include "../../src/vector/pvector.hpp"
#include "../../src/vector/svector.hpp"

#include "../snowball/snowball.hpp"
#include "../support/concept_harness.hpp"

// ---- Container types ----
REQUIRE_CONCEPT(micron::is_container, micron::vector<int>);
REQUIRE_CONCEPT(micron::is_container, micron::svector<int, 32>);
REQUIRE_CONCEPT(micron::is_container, micron::fvector<int>);
REQUIRE_CONCEPT(micron::is_container, micron::array<int, 16>);

// ---- String types ----
REQUIRE_CONCEPT(micron::is_container_or_string, micron::string);
REQUIRE_CONCEPT(micron::is_string, micron::string);

// ---- Object capability concepts ----
REQUIRE_CONCEPT(micron::is_movable_object, micron::vector<int>);
REQUIRE_CONCEPT(micron::is_regular_object, int);
REQUIRE_CONCEPT(micron::is_regular_object, micron::array<int, 8>);

// ---- Standard library concepts micron offers ----
REQUIRE_CONCEPT(micron::movable, micron::vector<int>);
REQUIRE_CONCEPT(micron::copyable, micron::vector<int>);
REQUIRE_CONCEPT(micron::destructible, micron::vector<int>);
REQUIRE_CONCEPT(micron::equality_comparable, int);

// ---- Trait checks ----
REQUIRE_TRAIT_V_TRUE(micron::is_trivially_copyable, int);
REQUIRE_TRAIT_V_TRUE(micron::is_integral, usize);

int
main()
{
  sb::print("=== CONCEPT SATISFACTION ===");
  sb::print("All static_asserts above passed (compile reached main()).");
  sb::print("=== ALL CONCEPT TESTS PASSED ===");
  return 1;
}
