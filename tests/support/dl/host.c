/* Copyright (c) 2026 David Lucius Severus. Boost Software License 1.0. */
extern int host_answer(void);
extern void host_event(int);
static __thread int counter = 40;

int
host_call(void)
{
  return host_answer();
}

int
tls_next(void)
{
  return ++counter;
}

__attribute__((constructor)) static void
init(void)
{
  host_event(10);
}

__attribute__((destructor)) static void
fini(void)
{
  host_event(counter);
}
