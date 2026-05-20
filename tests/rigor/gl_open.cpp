

#include "../../src/gfx/gl/gl.hpp"
#include "../../src/syscall.hpp"

extern "C" void *XOpenDisplay(const char *);
extern "C" void *glXGetProcAddress(const unsigned char *);
extern "C" void *eglGetDisplay(void *);
extern "C" void *wl_display_connect(const char *);
extern "C" void *wl_egl_window_create(void *, int, int);

namespace
{
[[maybe_unused]] void *const __force_link[] = {
  reinterpret_cast<void *>(&XOpenDisplay),       reinterpret_cast<void *>(&glXGetProcAddress),    reinterpret_cast<void *>(&eglGetDisplay),
  reinterpret_cast<void *>(&wl_display_connect), reinterpret_cast<void *>(&wl_egl_window_create),
};

void
note(const char *s) noexcept
{
  usize n = 0;
  while ( s[n] ) ++n;
  micron::syscall(SYS_write, 1, s, n);
  const char nl = '\n';
  micron::syscall(SYS_write, 1, &nl, 1);
}
}      // namespace

int
main()
{
  note("=== GL OPEN DIAG ===");
  try {
    note("[1] display");
    micron::gl::display dpy;
    note("[2] window");
    micron::gl::context_hints hints;
    micron::gl::window win(dpy, 320, 240, "diag", hints);
    note("[3] context");
    micron::gl::context ctx(win, hints);
    note("[4] destruct now");
  } catch ( const micron::except::__base_exception &e ) {
    note("CAUGHT in main:");
    note(e.what());
  }
  note("[5] all destructors finished");
  return 1;
}
