// POISON CELL -- Phase 4.
// /usr/include/micron is a PRE-PHASE-3 snapshot: it has no port/ at all and still has linux/.
// Every crt link cell in this file compiles /usr/src/mc_start/start.cpp, which resolves <micron/...>
// off the include path -- so without -I. they were all validating the installed tree, not this
// branch. This TU fails to compile the moment that regresses.
#include <micron/port/port.hpp>
int main() { return micron::port::cpu_id() >= 0 ? 0 : 1; }
