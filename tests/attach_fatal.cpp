#include <micron/attach/landlord.hpp>
#include <micron/exit.hpp>
#include <micron/io/console.hpp>
static int g_fatal_seen = 0;
extern "C" void my_fatal(int code) noexcept { g_fatal_seen = code; micron::console("FATAL HOOK ran, code=", code); micron::sys_exit(0); }
int main(){
  micron::__micron_attach_fatal = &my_fatal;
  micron::console("calling group_exit(77) with hook armed...");
  micron::group_exit(77);   // must divert to my_fatal, which sys_exit(0)s this thread
  micron::console("UNREACHABLE - hook failed");
  return 99;
}
