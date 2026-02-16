#pragma once

#include "../atomic/atomic.hpp"

#include "../memory/actions.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"

// a c++26 style rcu prototype
// not sure how long i'll keep this around

namespace micron
{

struct rcu_reader_state {
  atomic_token<size_t> epoch;
  atomic_token<bool> active;

  rcu_reader_state() : epoch(0), active(false) {}
};

template <typename T> struct rcu_tls_state {
  static thread_local rcu_reader_state state;
};

template <typename T> thread_local rcu_reader_state rcu_tls_state<T>::state;

template <typename T = void> class rcu_domain
{
  atomic_token<size_t> global_epoch;
  atomic_token<size_t> sync_epoch;

  struct retire_entry {
    void (*deleter)(void *);
    void *ptr;
    size_t retire_epoch;
    retire_entry *next;

    retire_entry(void (*d)(void *), void *p, size_t e) : deleter(d), ptr(p), retire_epoch(e), next(nullptr) {}
  };

  atomic<retire_entry *> retire_head;
  atomic<retire_entry *> retire_tail;
  atomic_token<size_t> retire_count;

  static constexpr size_t MAX_READERS = 256;
  atomic<rcu_reader_state *> readers[MAX_READERS];
  atomic_token<size_t> reader_count;

  atomic_token<bool> queue_lock;

  void
  lock_queue()
  {
    while ( !queue_lock.compare_and_swap(ATOMIC_OPEN, ATOMIC_LOCKED) ) {
      __cpu_pause();
    }
  }

  void
  unlock_queue()
  {
    queue_lock.store(ATOMIC_OPEN);
  }

  void
  register_reader()
  {
    auto &state = rcu_tls_state<T>::state;

    size_t idx = reader_count.fetch_add(1, memory_order_acq_rel);
    if ( idx >= MAX_READERS ) {
      reader_count.fetch_sub(1, memory_order_acq_rel);
      return;
    }

    readers[idx].operator=(&state);
  }

  void
  wait_for_readers(size_t target_epoch)
  {
    size_t num_readers = reader_count.get(memory_order_acquire);

    for ( ;; ) {
      bool all_clear = true;

      for ( size_t i = 0; i < num_readers; ++i ) {
        rcu_reader_state *reader = *readers[i].get();
        if ( !reader )
          continue;

        bool is_active = reader->active.get(memory_order_acquire);
        if ( !is_active )
          continue;

        size_t reader_epoch = reader->epoch.get(memory_order_acquire);
        if ( reader_epoch < target_epoch ) {
          all_clear = false;
          break;
        }
      }

      if ( all_clear )
        break;

      __cpu_pause();
    }
  }

  void
  process_retirements(size_t safe_epoch)
  {
    lock_queue();

    retire_entry *prev = nullptr;
    retire_entry *curr = *retire_head.get();

    while ( curr ) {
      if ( curr->retire_epoch < safe_epoch ) {
        retire_entry *to_delete = curr;

        if ( prev ) {
          prev->next = curr->next;
        } else {
          retire_head.operator=(curr->next);
        }

        retire_entry *next = curr->next;

        unlock_queue();

        to_delete->deleter(to_delete->ptr);
        delete to_delete;
        retire_count.fetch_sub(1, memory_order_acq_rel);

        lock_queue();

        curr = next;
        if ( !prev && curr ) {
          retire_head.operator=(curr);
        }
      } else {
        prev = curr;
        curr = curr->next;
      }
    }

    if ( !*retire_head.get() ) {
      retire_tail.operator=(static_cast<retire_entry *>(nullptr));
    }

    unlock_queue();
  }

public:
  rcu_domain() : global_epoch(0), sync_epoch(0), retire_count(0), queue_lock(ATOMIC_OPEN), reader_count(0)
  {
    retire_head.operator=(static_cast<retire_entry *>(nullptr));
    retire_tail.operator=(static_cast<retire_entry *>(nullptr));

    for ( size_t i = 0; i < MAX_READERS; ++i ) {
      readers[i].operator=(static_cast<rcu_reader_state *>(nullptr));
    }
  }

  ~rcu_domain() { rcu_barrier(*this); }

  void
  lock()
  {
    auto &state = rcu_tls_state<T>::state;

    if ( !state.active.get(memory_order_relaxed) ) {
      register_reader();
    }

    state.active.store(true, memory_order_release);
    state.epoch.store(global_epoch.get(memory_order_acquire), memory_order_release);
  }

  void
  unlock()
  {
    auto &state = rcu_tls_state<T>::state;
    state.active.store(false, memory_order_release);
  }

  class scoped_lock
  {
    rcu_domain &domain;

  public:
    explicit scoped_lock(rcu_domain &d) : domain(d) { domain.lock(); }

    ~scoped_lock() { domain.unlock(); }

    scoped_lock(const scoped_lock &) = delete;
    scoped_lock &operator=(const scoped_lock &) = delete;
  };

  size_t
  get_epoch() const
  {
    return global_epoch.get(memory_order_acquire);
  }

  void
  advance_epoch()
  {
    global_epoch.fetch_add(1, memory_order_acq_rel);
  }

  template <typename U>
  void
  retire_impl(U *ptr, void (*deleter)(void *))
  {
    size_t current_epoch = global_epoch.get(memory_order_acquire);
    auto *entry = new retire_entry(deleter, static_cast<void *>(ptr), current_epoch);

    lock_queue();

    retire_entry *tail = *retire_tail.get();
    if ( tail ) {
      tail->next = entry;
    } else {
      retire_head.operator=(entry);
    }
    retire_tail.operator=(entry);
    retire_count.fetch_add(1, memory_order_acq_rel);

    unlock_queue();

    if ( retire_count.get(memory_order_acquire) > 16 ) {
      advance_epoch();
      size_t safe_epoch = current_epoch;
      wait_for_readers(current_epoch + 1);
      process_retirements(safe_epoch);
    }
  }

  void
  synchronize_impl()
  {
    size_t current = global_epoch.fetch_add(1, memory_order_acq_rel);
    wait_for_readers(current + 1);
    sync_epoch.store(current + 1, memory_order_release);
  }

  void
  barrier_impl()
  {
    size_t current = global_epoch.fetch_add(1, memory_order_acq_rel);

    wait_for_readers(current + 1);

    process_retirements(current + 1);

    sync_epoch.store(current + 1, memory_order_release);
  }

  size_t
  pending_retirements() const
  {
    return retire_count.get(memory_order_acquire);
  }

  rcu_domain(const rcu_domain &) = delete;
  rcu_domain(rcu_domain &&) = delete;
  rcu_domain &operator=(const rcu_domain &) = delete;
};

using rcu_default_domain = rcu_domain<void>;
inline rcu_default_domain default_rcu_domain;

template <typename T = void>
void
rcu_synchronize(rcu_domain<T> &domain)
{
  domain.synchronize_impl();
}

inline void
rcu_synchronize()
{
  rcu_synchronize(default_rcu_domain);
}

template <typename T = void>
void
rcu_barrier(rcu_domain<T> &domain)
{
  domain.barrier_impl();
}

inline void
rcu_barrier()
{
  rcu_barrier(default_rcu_domain);
}

template <typename T, typename D, typename Domain = void>
void
rcu_retire(T *ptr, D deleter, rcu_domain<Domain> &domain)
{
  auto wrapper = [](void *p) {
    D del;
    del(static_cast<T *>(p));
  };

  domain.retire_impl(ptr, wrapper);
}

template <typename T, typename D>
void
rcu_retire(T *ptr, D deleter)
{
  rcu_retire(ptr, deleter, default_rcu_domain);
}

template <typename T, typename Domain = void>
void
rcu_retire(T *ptr, rcu_domain<Domain> &domain)
{
  auto deleter = [](void *p) { delete static_cast<T *>(p); };

  domain.retire_impl(ptr, deleter);
}

template <typename T>
void
rcu_retire(T *ptr)
{
  rcu_retire(ptr, default_rcu_domain);
}

template <typename T> class rcu_obj_base
{
protected:
  virtual ~rcu_obj_base() = default;

public:
  void
  retire()
  {
    rcu_retire(static_cast<T *>(this));
  }

  template <typename Domain>
  void
  retire(rcu_domain<Domain> &domain)
  {
    rcu_retire(static_cast<T *>(this), domain);
  }
};

class rcu_reader
{
public:
  rcu_reader() { default_rcu_domain.lock(); }

  ~rcu_reader() { default_rcu_domain.unlock(); }

  rcu_reader(const rcu_reader &) = delete;
  rcu_reader &operator=(const rcu_reader &) = delete;
};

template <typename T> class rcu_reader_domain
{
  rcu_domain<T> &domain;

public:
  explicit rcu_reader_domain(rcu_domain<T> &d) : domain(d) { domain.lock(); }

  ~rcu_reader_domain() { domain.unlock(); }

  rcu_reader_domain(const rcu_reader_domain &) = delete;
  rcu_reader_domain &operator=(const rcu_reader_domain &) = delete;
};

template <typename T, typename Domain = void> class rcu_ptr
{
  atomic<T *> ptr;
  rcu_domain<Domain> *domain;

public:
  rcu_ptr() : domain(&default_rcu_domain) { ptr.operator=(static_cast<T *>(nullptr)); }

  explicit rcu_ptr(rcu_domain<Domain> &d) : domain(&d) { ptr.operator=(static_cast<T *>(nullptr)); }

  rcu_ptr(T *p, rcu_domain<Domain> &d) : domain(&d) { ptr.operator=(p); }

  T *
  load() const
  {
    auto *p = ptr.get();
    return *p;
  }

  void
  store(T *new_ptr)
  {
    auto *old_ptr_ref = ptr.get();
    T *old = *old_ptr_ref;
    ptr.release();

    ptr.operator=(new_ptr);

    if ( old ) {
      rcu_retire(old, *domain);
    }
  }

  T *
  exchange(T *new_ptr)
  {
    auto *old_ptr_ref = ptr.get();
    T *old = *old_ptr_ref;
    ptr.release();

    ptr.operator=(new_ptr);

    if ( old ) {
      rcu_retire(old, *domain);
    }

    return old;
  }

  bool
  compare_exchange(T *expected, T *desired)
  {
    auto *p = ptr.get();
    T *current = *p;

    if ( current == expected ) {
      ptr.release();
      ptr.operator=(desired);

      if ( expected ) {
        rcu_retire(expected, *domain);
      }
      return true;
    }

    ptr.release();
    return false;
  }

  rcu_ptr(const rcu_ptr &) = delete;
  rcu_ptr &operator=(const rcu_ptr &) = delete;
};

template <typename F, typename Domain = void>
void
rcu_call(F &&func, rcu_domain<Domain> &domain)
{
  struct wrapper {
    F f;
    wrapper(F &&fn) : f(micron::forward<F>(fn)) {}
  };

  auto *w = new wrapper(micron::forward<F>(func));

  auto deleter = [](void *p) {
    auto *wp = static_cast<wrapper *>(p);
    wp->f();
    delete wp;
  };

  domain.retire_impl(w, deleter);
}

template <typename F>
void
rcu_call(F &&func)
{
  rcu_call(micron::forward<F>(func), default_rcu_domain);
}

template <typename T, typename Domain = void> class rcu_batch
{
  rcu_domain<Domain> &domain;
  T **objects;
  size_t count;
  size_t capacity;

  void
  grow()
  {
    size_t new_cap = capacity * 2;
    T **new_objs = new T *[new_cap];

    for ( size_t i = 0; i < count; ++i ) {
      new_objs[i] = objects[i];
    }

    delete[] objects;
    objects = new_objs;
    capacity = new_cap;
  }

public:
  explicit rcu_batch(rcu_domain<Domain> &d, size_t initial_capacity = 16) : domain(d), count(0), capacity(initial_capacity)
  {
    objects = new T *[capacity];
  }

  ~rcu_batch()
  {
    flush();
    delete[] objects;
  }

  void
  add(T *ptr)
  {
    if ( count >= capacity ) {
      grow();
    }
    objects[count++] = ptr;
  }

  void
  flush()
  {
    for ( size_t i = 0; i < count; ++i ) {
      rcu_retire(objects[i], domain);
    }
    count = 0;
  }

  size_t
  size() const
  {
    return count;
  }

  rcu_batch(const rcu_batch &) = delete;
  rcu_batch &operator=(const rcu_batch &) = delete;
};

};     // namespace micron
