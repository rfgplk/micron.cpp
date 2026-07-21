

#include <micron/attach/landlord.hpp>
#include <micron/io/console.hpp>
#include <micron/thread/spawn.hpp>

static unsigned char g_mod_tdata[16] = { 0xAB, 0xCD, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x11, 0x22, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

static inline unsigned char
rd(long long bias, long long k)
{
  unsigned char *p;
  asm volatile("mov %%fs:0,%0" : "=r"(p));
  return *(p + bias + k);
}

static inline void
wr(long long bias, long long k, unsigned char v)
{
  unsigned char *p;
  asm volatile("mov %%fs:0,%0" : "=r"(p));
  *(p + bias + k) = v;
}

int
main()
{
  int fail = 0;
  u64 tok = 0;
  i64 bias = 0;
  u64 off = 0;
  if ( micron::__attach_tls_assign(g_mod_tdata, 16, 16, 16, &tok, &bias, &off) != 0 ) {
    micron::console("FAIL assign");
    return 1;
  }
  micron::console("assign ok, surplus_off=", (int)off, " token=", (int)tok);
  if ( micron::__attach_tls_seed_current(tok) != 0 ) {
    micron::console("FAIL seed");
    return 2;
  }

  if ( rd(bias, 0) != 0xAB || rd(bias, 1) != 0xCD || rd(bias, 8) != 0x11 || rd(bias, 9) != 0x22 ) {
    micron::console("FAIL main tp-rel read");
    fail |= 1;
  } else
    micron::console("main tp-rel read OK");

  int wdone = 0, wok = 0;
  auto &h = micron::go([&] {
    bool ok = (rd(bias, 0) == 0xAB && rd(bias, 8) == 0x11);
    wr(bias, 0, 0x77);
    ok = ok && (rd(bias, 0) == 0x77);
    __atomic_store_n(&wok, ok ? 1 : 2, __ATOMIC_RELEASE);
    __atomic_store_n(&wdone, 1, __ATOMIC_RELEASE);
  });
  (void)h;
  while ( __atomic_load_n(&wdone, __ATOMIC_ACQUIRE) == 0 ) micron::yield();
  if ( __atomic_load_n(&wok, __ATOMIC_ACQUIRE) != 1 ) {
    micron::console("FAIL worker read");
    fail |= 2;
  } else
    micron::console("worker tp-rel read OK");

  if ( rd(bias, 0) != 0xAB ) {
    micron::console("FAIL isolation (main clobbered)");
    fail |= 4;
  } else
    micron::console("per-thread isolation OK");

  micron::console(fail == 0 ? "ALL PASS" : "SOME FAIL");
  return fail;
}
