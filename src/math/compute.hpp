//  Copyright (c) 2024- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt
#pragma once

#include "../atomic/atomic.hpp"
#include "../concepts.hpp"
#include "../except.hpp"
#include "../memory/allocation/abcmalloc/malloc.hpp"
#include "../memory/cmemory.hpp"
#include "../new.hpp"
#include "../slice.hpp"
#include "../sync/futex.hpp"
#include "../type_traits.hpp"
#include "../types.hpp"
#include "../vector/vector.hpp"

namespace micron::math::compute
{

enum class compute_status : u8 {
  ok = 0,
  invalid_plan,
  invalid_tensor,
  invalid_binding,
  incompatible_binding,
  missing_binding,
  allocation_failure,
  domain_failure,
  dependency_failed,
  executor_failure,
  kernel_failure,
  overflow
};

enum class tensor_access : u8 { read_only = 0, read_write };
enum class alias_mode : u8 { none = 0, read_only_view, destructive_in_place };

struct tensor_id {
  u32 value{ invalid_value() };

  [[nodiscard]] static constexpr u32
  invalid_value() noexcept
  {
    return micron::numeric_limits<u32>::max();
  }

  [[nodiscard]] static constexpr tensor_id
  invalid() noexcept
  {
    return {};
  }

  [[nodiscard]] constexpr bool
  valid() const noexcept
  {
    return value != invalid_value();
  }

  friend constexpr bool operator==(tensor_id, tensor_id) noexcept = default;

  friend constexpr bool
  operator<(tensor_id left, tensor_id right) noexcept
  {
    return left.value < right.value;
  }
};

struct operation_id {
  u32 value{ invalid_value() };

  [[nodiscard]] static constexpr u32
  invalid_value() noexcept
  {
    return micron::numeric_limits<u32>::max();
  }

  [[nodiscard]] static constexpr operation_id
  invalid() noexcept
  {
    return {};
  }

  [[nodiscard]] constexpr bool
  valid() const noexcept
  {
    return value != invalid_value();
  }

  friend constexpr bool operator==(operation_id, operation_id) noexcept = default;
};

template<typename Dtype> struct tensor_descriptor {
  Dtype dtype{};
  u16 domain{};
  micron::vector<usize, micron::allocator_serial<>, false> dimensions;
  micron::vector<ssize_t, micron::allocator_serial<>, false> strides;
  usize byte_size{};
  usize alignment{};
  tensor_access access{ tensor_access::read_only };

  [[nodiscard]] usize
  rank() const noexcept
  {
    return dimensions.size();
  }

  [[nodiscard]] bool
  scalar() const noexcept
  {
    return dimensions.empty();
  }
};

struct alias_declaration {
  alias_mode mode{ alias_mode::none };
  usize input_index{};
  usize offset_bytes{};
};

template<typename Dtype> struct tensor_view {
  void *data{};
  const tensor_descriptor<Dtype> *descriptor{};
  tensor_access access{ tensor_access::read_only };

  template<typename T>
  [[nodiscard]] T *
  as() noexcept
  {
    return reinterpret_cast<T *>(data);
  }

  template<typename T>
  [[nodiscard]] const T *
  as() const noexcept
  {
    return reinterpret_cast<const T *>(data);
  }

  [[nodiscard]] explicit
  operator bool() const noexcept
  {
    return data != nullptr || (descriptor && descriptor->byte_size == 0);
  }
};

struct compute_result {
  compute_status status{ compute_status::ok };
  operation_id failing_operation{};

  [[nodiscard]] bool
  succeeded() const noexcept
  {
    return status == compute_status::ok;
  }

  [[nodiscard]] explicit
  operator bool() const noexcept
  {
    return succeeded();
  }
};

struct host_domains {
  using domain_type = u16;
  static constexpr domain_type host = 0;

  [[nodiscard]] static constexpr bool
  valid(domain_type domain) noexcept
  {
    return domain == host;
  }

  [[nodiscard]] static constexpr usize
  minimum_alignment(domain_type) noexcept
  {
    return alignof(max_align_t);
  }

  [[nodiscard]] static void *
  allocate(domain_type domain, usize bytes, usize alignment) noexcept
  {
    if ( !valid(domain) || bytes == 0 || alignment == 0 || (alignment & (alignment - 1)) != 0 ) return nullptr;
    usize padded{};
    if ( __builtin_add_overflow(bytes, alignment - 1, &padded) ) return nullptr;
    padded &= ~(alignment - 1);
    return abc::aligned_alloc(alignment, padded);
  }

  static void
  release(domain_type, void *pointer, usize, usize alignment) noexcept
  {
    if ( !pointer ) return;
    if ( alignment <= abc::__hdr_offset )
      abc::dealloc(reinterpret_cast<byte *>(pointer));
    else
      abc::aligned_free(pointer);
  }

  [[nodiscard]] static compute_status
  copy(domain_type destination_domain, void *destination, domain_type source_domain, const void *source, usize bytes) noexcept
  {
    if ( !valid(destination_domain) || !valid(source_domain) || (!destination && bytes) || (!source && bytes) )
      return compute_status::domain_failure;
    if ( bytes ) micron::memcpy(reinterpret_cast<byte *>(destination), reinterpret_cast<const byte *>(source), bytes);
    return compute_status::ok;
  }

  [[nodiscard]] static constexpr compute_status
  synchronize(domain_type) noexcept
  {
    return compute_status::ok;
  }
};

template<typename Domains>
concept domain_provider
    = requires(typename Domains::domain_type domain, void *destination, const void *source, usize bytes, usize alignment) {
        { Domains::valid(domain) } -> micron::convertible_to<bool>;
        { Domains::minimum_alignment(domain) } -> micron::convertible_to<usize>;
        { Domains::allocate(domain, bytes, alignment) } -> micron::same_as<void *>;
        Domains::release(domain, destination, bytes, alignment);
        { Domains::copy(domain, destination, domain, source, bytes) } -> micron::same_as<compute_status>;
        { Domains::synchronize(domain) } -> micron::same_as<compute_status>;
      };

namespace __impl
{

template<typename T> using __compute_vec = micron::vector<T, micron::allocator_serial<>, false>;

template<domain_provider Domains> struct __tensor_storage_control {
  micron::atomic_token<usize> references{ usize(1) };
  void *pointer{};
  usize bytes{};
  usize alignment{};
  typename Domains::domain_type domain{};

  __tensor_storage_control(void *p, usize n, usize a, typename Domains::domain_type d) noexcept
      : pointer(p), bytes(n), alignment(a), domain(d)
  {
  }
};

};      // namespace __impl

template<domain_provider Domains = host_domains> class basic_tensor_storage
{
  using control_type = __impl::__tensor_storage_control<Domains>;
  control_type *__control{};

  explicit basic_tensor_storage(control_type *control) noexcept : __control(control) { }

  void
  __release() noexcept
  {
    if ( !__control ) return;
    if ( __control->references.fetch_sub(usize(1), micron::memory_order_acq_rel) == 1 ) {
      Domains::release(__control->domain, __control->pointer, __control->bytes, __control->alignment);
      delete __control;
    }
    __control = nullptr;
  }

public:
  using domain_type = typename Domains::domain_type;

  basic_tensor_storage() noexcept = default;

  ~basic_tensor_storage() { __release(); }

  basic_tensor_storage(const basic_tensor_storage &other) noexcept : __control(other.__control)
  {
    if ( __control ) __control->references.fetch_add(usize(1), micron::memory_order_relaxed);
  }

  basic_tensor_storage(basic_tensor_storage &&other) noexcept : __control(other.__control) { other.__control = nullptr; }

  basic_tensor_storage &
  operator=(const basic_tensor_storage &other) noexcept
  {
    if ( this == micron::addressof(other) ) return *this;
    if ( other.__control ) other.__control->references.fetch_add(usize(1), micron::memory_order_relaxed);
    __release();
    __control = other.__control;
    return *this;
  }

  basic_tensor_storage &
  operator=(basic_tensor_storage &&other) noexcept
  {
    if ( this == micron::addressof(other) ) return *this;
    __release();
    __control = other.__control;
    other.__control = nullptr;
    return *this;
  }

  [[nodiscard]] static basic_tensor_storage
  allocate(domain_type domain, usize bytes, usize alignment)
  {
    if ( bytes == 0 ) return basic_tensor_storage(new control_type(nullptr, 0, alignment, domain));
    void *pointer = Domains::allocate(domain, bytes, alignment);
    if ( !pointer ) return {};
#if !defined(__micron_freestanding) || defined(__micron_eh)
    try {
      return basic_tensor_storage(new control_type(pointer, bytes, alignment, domain));
    } catch ( ... ) {
      Domains::release(domain, pointer, bytes, alignment);
      throw;
    }
#else
    return basic_tensor_storage(new control_type(pointer, bytes, alignment, domain));
#endif
  }

  [[nodiscard]] void *
  data() noexcept
  {
    return __control ? __control->pointer : nullptr;
  }

  [[nodiscard]] const void *
  data() const noexcept
  {
    return __control ? __control->pointer : nullptr;
  }

  [[nodiscard]] usize
  size() const noexcept
  {
    return __control ? __control->bytes : 0;
  }

  [[nodiscard]] usize
  alignment() const noexcept
  {
    return __control ? __control->alignment : 0;
  }

  [[nodiscard]] domain_type
  domain() const noexcept
  {
    return __control ? __control->domain : domain_type{};
  }

  [[nodiscard]] usize
  use_count() const noexcept
  {
    return __control ? __control->references.get(micron::memory_order_acquire) : 0;
  }

  [[nodiscard]] explicit
  operator bool() const noexcept
  {
    return __control != nullptr;
  }
};

using tensor_storage = basic_tensor_storage<host_domains>;

template<typename Dtype, domain_provider Domains = host_domains> struct shared_tensor {
  basic_tensor_storage<Domains> storage;
  tensor_descriptor<Dtype> descriptor;
  usize offset_bytes{};

  [[nodiscard]] void *
  data() noexcept
  {
    void *base = storage.data();
    return base ? reinterpret_cast<byte *>(base) + offset_bytes : nullptr;
  }

  [[nodiscard]] const void *
  data() const noexcept
  {
    const void *base = storage.data();
    return base ? reinterpret_cast<const byte *>(base) + offset_bytes : nullptr;
  }

  [[nodiscard]] explicit
  operator bool() const noexcept
  {
    return static_cast<bool>(storage);
  }
};

class inline_executor
{
public:
  [[nodiscard]] bool
  submit(void (*function)(void *) noexcept, void *argument) noexcept
  {
    function(argument);
    return true;
  }
};

template<typename Ops, domain_provider Domains = host_domains> class compute_graph;
template<typename Ops, domain_provider Domains = host_domains> class compute_plan;
template<typename Ops, domain_provider Domains = host_domains> class compute_session;

};      // namespace micron::math::compute

namespace micron::math::compute
{

template<typename Ops>
concept __compute_session_registry
    = requires(typename Ops::operation_type operation, typename Ops::dtype_type dtype, typename Ops::context_type context,
               micron::raw_slice<const tensor_descriptor<typename Ops::dtype_type>> input_descriptors,
               micron::raw_slice<tensor_descriptor<typename Ops::dtype_type>> output_descriptors,
               micron::raw_slice<const tensor_view<typename Ops::dtype_type>> input_views,
               micron::raw_slice<tensor_view<typename Ops::dtype_type>> output_views) {
        typename Ops::operation_type;
        typename Ops::dtype_type;
        typename Ops::context_type;
        { Ops::dtype_size(dtype) } -> micron::convertible_to<usize>;
        { Ops::output_count(operation) } -> micron::convertible_to<usize>;
        { Ops::infer(operation, input_descriptors, output_descriptors) } -> micron::same_as<compute_status>;
        { Ops::validate(operation, input_descriptors, output_descriptors) } -> micron::same_as<compute_status>;
        { Ops::execute(operation, context, input_views, output_views) } noexcept -> micron::same_as<compute_status>;
      };

namespace __impl
{

void __compute_plan_error(const char *message);
template<typename Dtype> bool __descriptor_compatible(const tensor_descriptor<Dtype> &, const tensor_descriptor<Dtype> &) noexcept;
template<typename Ops, domain_provider Domains>
bool __normalize_descriptor(tensor_descriptor<typename Ops::dtype_type> &, ssize_t &, ssize_t &) noexcept;

template<domain_provider Domains> struct __runtime_tensor {
  basic_tensor_storage<Domains> storage;
  void *pointer{};
  bool valid{};
  bool borrowed{};
};

template<typename Session> struct __compute_job {
  Session *session{};
  u32 operation{};
};

};      // namespace __impl

template<typename Ops, domain_provider Domains> class compute_session
{
  static_assert(__compute_session_registry<Ops>, "compute_session requires a compile-time operation registry");

public:
  using plan_type = compute_plan<Ops, Domains>;
  using operation_type = typename Ops::operation_type;
  using dtype_type = typename Ops::dtype_type;
  using context_type = typename Ops::context_type;
  using descriptor_type = tensor_descriptor<dtype_type>;
  using storage_type = basic_tensor_storage<Domains>;
  using shared_tensor_type = shared_tensor<dtype_type, Domains>;

private:
  using runtime_tensor = __impl::__runtime_tensor<Domains>;
  using job_type = __impl::__compute_job<compute_session>;

  const plan_type *__plan{};
  context_type __context;
  __impl::__compute_vec<storage_type> __slot_storage;
  __impl::__compute_vec<runtime_tensor> __runtime;
  __impl::__compute_vec<tensor_view<dtype_type>> __input_views;
  __impl::__compute_vec<tensor_view<dtype_type>> __output_views;
  __impl::__compute_vec<job_type> __jobs;
  __impl::__compute_vec<micron::atomic_token<u32>> __pending;
  __impl::__compute_vec<micron::atomic_token<u8>> __failed_dependency;
  __impl::__compute_vec<compute_status> __operation_status;
  micron::atomic_token<u32> __remaining{ 0 };
  micron::atomic_token<u32> __earliest_failure{ operation_id::invalid_value() };
  void *__executor{};
  bool (*__submit)(void *, void (*)(void *) noexcept, void *) noexcept {};
  bool __executing{};

  [[nodiscard]] bool
  __valid_tensor(tensor_id tensor) const noexcept
  {
    return __plan && tensor.valid() && tensor.value < __plan->__tensors.size();
  }

  [[nodiscard]] static usize
  __logical_origin(const descriptor_type &descriptor, ssize_t minimum) noexcept
  {
    (void)descriptor;
    return minimum < 0 ? static_cast<usize>(-(minimum + 1)) + 1 : 0;
  }

  void
  __refresh_runtime_pointers() noexcept
  {
    for ( usize tensor = 0; tensor < __plan->__tensors.size(); ++tensor ) {
      const auto &meta = __plan->__tensors.data()[tensor];
      if ( meta.root.value == tensor ) continue;
      auto &runtime = __runtime.data()[tensor];
      const auto &root = __runtime.data()[meta.root.value];
      runtime.storage = root.storage;
      runtime.borrowed = root.borrowed;
      runtime.pointer = root.pointer ? reinterpret_cast<byte *>(root.pointer) + meta.root_offset : nullptr;
    }
  }

  void
  __initialize()
  {
    static_assert(noexcept(Ops::execute(micron::declval<const operation_type &>(), micron::declval<context_type &>(),
                                        micron::declval<micron::raw_slice<const tensor_view<dtype_type>>>(),
                                        micron::declval<micron::raw_slice<tensor_view<dtype_type>>>())),
                  "Ops::execute must be noexcept");
    if ( !__plan ) __impl::__compute_plan_error("compute_session: null plan");
    __slot_storage.reserve(__plan->__slots.size());
    for ( const auto &slot : __plan->__slots ) {
      auto storage = storage_type::allocate(slot.domain, slot.bytes, slot.alignment);
      if ( !storage ) micron::exc<micron::except::memory_error>("compute_session: arena allocation failed");
      if ( slot.bytes ) micron::memset(storage.data(), byte(0), slot.bytes);
      __slot_storage.push_back(micron::move(storage));
    }

    __runtime.resize(__plan->__tensors.size());
    for ( usize tensor = 0; tensor < __plan->__tensors.size(); ++tensor ) {
      const auto &meta = __plan->__tensors.data()[tensor];
      if ( meta.root.value != tensor || meta.external || meta.slot == micron::numeric_limits<u32>::max() ) continue;
      auto &runtime = __runtime.data()[tensor];
      runtime.storage = __slot_storage.data()[meta.slot];
      runtime.pointer = runtime.storage.data()
                            ? reinterpret_cast<byte *>(runtime.storage.data()) + __logical_origin(meta.descriptor, meta.minimum_offset)
                            : nullptr;
      runtime.valid = meta.state;
    }
    __refresh_runtime_pointers();

    __input_views.resize(__plan->__inputs.size());
    __output_views.resize(__plan->__outputs.size());
    __jobs.resize(__plan->__operations.size());
    __pending.resize(__plan->__operations.size(), micron::atomic_token<u32>(0));
    __failed_dependency.resize(__plan->__operations.size(), micron::atomic_token<u8>(0));
    __operation_status.resize(__plan->__operations.size(), compute_status::ok);
    for ( usize operation = 0; operation < __jobs.size(); ++operation ) __jobs.data()[operation] = { this, static_cast<u32>(operation) };
  }

  void
  __publish_failure(u32 operation, compute_status status) noexcept
  {
    __operation_status.data()[operation] = status;
    u32 current = __earliest_failure.get(micron::memory_order_acquire);
    while ( operation < current
            && !__earliest_failure.compare_exchange_weak(current, operation, micron::memory_order_acq_rel, micron::memory_order_acquire) ) {
    }
  }

  void __schedule(u32 operation) noexcept;

  void
  __complete(u32 operation, compute_status status, bool dependency_failure) noexcept
  {
    const auto &node = __plan->__operations.data()[operation];
    const bool failed = status != compute_status::ok;
    if ( failed ) {
      if ( !dependency_failure ) __publish_failure(operation, status);
    } else {
      __operation_status.data()[operation] = compute_status::ok;
      for ( usize output = 0; output < node.output_count; ++output )
        __runtime.data()[__plan->__outputs.data()[node.output_begin + output].value].valid = true;
    }
    for ( usize i = 0; i < node.dependent_count; ++i ) {
      const u32 dependent = __plan->__dependents.data()[node.dependent_begin + i];
      if ( failed ) __failed_dependency.data()[dependent].store(u8(1), micron::memory_order_release);
      if ( __pending.data()[dependent].fetch_sub(u32(1), micron::memory_order_acq_rel) == 1 ) __schedule(dependent);
    }
    if ( __remaining.fetch_sub(u32(1), micron::memory_order_acq_rel) == 1 ) micron::wake_futex(__remaining.ptr(), 0x7fffffff);
  }

  void
  __run_operation(u32 operation) noexcept
  {
    if ( __failed_dependency.data()[operation].get(micron::memory_order_acquire) != 0 ) {
      __complete(operation, compute_status::dependency_failed, true);
      return;
    }
    const auto &node = __plan->__operations.data()[operation];
    const auto status
        = Ops::execute(node.operation, __context,
                       micron::raw_slice<const tensor_view<dtype_type>>(__input_views.data() + node.input_begin, node.input_count),
                       micron::raw_slice<tensor_view<dtype_type>>(__output_views.data() + node.output_begin, node.output_count));
    __complete(operation, status, false);
  }

  static void
  __job_entry(void *opaque) noexcept
  {
    auto *job = static_cast<job_type *>(opaque);
    job->session->__run_operation(job->operation);
  }

  template<typename Executor>
  [[nodiscard]] static bool
  __submit_adapter(void *opaque, void (*function)(void *) noexcept, void *argument) noexcept
  {
    return static_cast<Executor *>(opaque)->submit(function, argument);
  }

  [[nodiscard]] bool
  __bindings_ready() const noexcept
  {
    for ( usize tensor = 0; tensor < __plan->__tensors.size(); ++tensor ) {
      const auto &meta = __plan->__tensors.data()[tensor];
      if ( meta.external && meta.root.value == tensor && !__runtime.data()[tensor].valid ) return false;
    }
    return true;
  }

  void
  __prepare_execution() noexcept
  {
    for ( usize tensor = 0; tensor < __plan->__tensors.size(); ++tensor ) {
      const auto &meta = __plan->__tensors.data()[tensor];
      if ( !meta.external && !meta.state ) __runtime.data()[tensor].valid = false;
    }
    __refresh_runtime_pointers();
    for ( usize index = 0; index < __plan->__inputs.size(); ++index ) {
      const auto tensor = __plan->__inputs.data()[index];
      const auto &meta = __plan->__tensors.data()[tensor.value];
      __input_views.data()[index] = { __runtime.data()[tensor.value].pointer, micron::addressof(meta.descriptor), meta.descriptor.access };
    }
    for ( usize index = 0; index < __plan->__outputs.size(); ++index ) {
      const auto tensor = __plan->__outputs.data()[index];
      const auto &meta = __plan->__tensors.data()[tensor.value];
      __output_views.data()[index] = { __runtime.data()[tensor.value].pointer, micron::addressof(meta.descriptor), meta.descriptor.access };
    }
    for ( usize operation = 0; operation < __plan->__operations.size(); ++operation ) {
      __pending.data()[operation].store(__plan->__operations.data()[operation].dependency_count, micron::memory_order_relaxed);
      __failed_dependency.data()[operation].store(u8(0), micron::memory_order_relaxed);
      __operation_status.data()[operation] = compute_status::ok;
    }
    __earliest_failure.store(operation_id::invalid_value(), micron::memory_order_relaxed);
    __remaining.store(static_cast<u32>(__plan->__operations.size()), micron::memory_order_release);
  }

public:
  explicit compute_session(const plan_type &plan)
    requires micron::is_default_constructible_v<context_type>
      : __plan(micron::addressof(plan)), __context()
  {
    __initialize();
  }

  explicit compute_session(const plan_type &plan, const context_type &context) : __plan(micron::addressof(plan)), __context(context)
  {
    __initialize();
  }

  explicit compute_session(const plan_type &plan, context_type &&context)
      : __plan(micron::addressof(plan)), __context(micron::move(context))
  {
    __initialize();
  }

  compute_session(const compute_session &) = delete;
  compute_session &operator=(const compute_session &) = delete;
  compute_session(compute_session &&) = delete;
  compute_session &operator=(compute_session &&) = delete;

  [[nodiscard]] context_type &
  context() noexcept
  {
    return __context;
  }

  [[nodiscard]] const context_type &
  context() const noexcept
  {
    return __context;
  }

  [[nodiscard]] compute_status
  bind_borrowed(tensor_id tensor, void *pointer, const descriptor_type &descriptor) noexcept
  {
    if ( __executing || !__valid_tensor(tensor) ) return compute_status::invalid_tensor;
    const auto &meta = __plan->__tensors.data()[tensor.value];
    descriptor_type normalized = descriptor;
    ssize_t minimum{};
    ssize_t maximum{};
    if ( !__impl::__normalize_descriptor<Ops, Domains>(normalized, minimum, maximum) || !meta.external || meta.root != tensor
         || !__impl::__descriptor_compatible(meta.descriptor, normalized) )
      return meta.external ? compute_status::incompatible_binding : compute_status::invalid_binding;
    if ( meta.descriptor.byte_size && !pointer ) return compute_status::invalid_binding;
    if ( pointer && (reinterpret_cast<uintptr_t>(pointer) & (meta.descriptor.alignment - 1)) != 0 )
      return compute_status::incompatible_binding;
    auto &runtime = __runtime.data()[tensor.value];
    runtime.storage = {};
    runtime.pointer = pointer;
    runtime.borrowed = true;
    runtime.valid = true;
    __refresh_runtime_pointers();
    return compute_status::ok;
  }

  [[nodiscard]] compute_status
  bind_borrowed(tensor_id tensor, const void *pointer, const descriptor_type &descriptor) noexcept
  {
    if ( __valid_tensor(tensor) && __plan->__tensors.data()[tensor.value].descriptor.access == tensor_access::read_write )
      return compute_status::invalid_binding;
    return bind_borrowed(tensor, const_cast<void *>(pointer), descriptor);
  }

  [[nodiscard]] compute_status
  bind_shared(tensor_id tensor, const shared_tensor_type &binding) noexcept
  {
    if ( __executing || !__valid_tensor(tensor) || !binding ) return compute_status::invalid_binding;
    const auto &meta = __plan->__tensors.data()[tensor.value];
    descriptor_type normalized = binding.descriptor;
    ssize_t minimum{};
    ssize_t maximum{};
    if ( meta.root != tensor || (!meta.external && !meta.state) ) return compute_status::invalid_binding;
    if ( !__impl::__normalize_descriptor<Ops, Domains>(normalized, minimum, maximum)
         || !__impl::__descriptor_compatible(meta.descriptor, normalized)
         || binding.storage.domain() != static_cast<typename Domains::domain_type>(meta.descriptor.domain)
         || binding.offset_bytes > binding.storage.size() )
      return compute_status::incompatible_binding;
    const usize origin = __logical_origin(normalized, minimum);
    if ( origin > binding.offset_bytes || meta.descriptor.byte_size > binding.storage.size() - (binding.offset_bytes - origin) )
      return compute_status::incompatible_binding;
    void *pointer = const_cast<void *>(binding.data());
    if ( pointer && (reinterpret_cast<uintptr_t>(pointer) & (meta.descriptor.alignment - 1)) != 0 )
      return compute_status::incompatible_binding;
    auto &runtime = __runtime.data()[tensor.value];
    runtime.storage = binding.storage;
    runtime.pointer = pointer;
    runtime.borrowed = false;
    runtime.valid = true;
    __refresh_runtime_pointers();
    return compute_status::ok;
  }

  [[nodiscard]] compute_status
  copy_from(tensor_id tensor, typename Domains::domain_type source_domain, const void *source, usize bytes) noexcept
  {
    if ( __executing || !__valid_tensor(tensor) || !__runtime.data()[tensor.value].valid ) return compute_status::invalid_tensor;
    const auto &meta = __plan->__tensors.data()[tensor.value];
    if ( meta.descriptor.access != tensor_access::read_write || bytes != meta.descriptor.byte_size ) return compute_status::invalid_binding;
    return Domains::copy(static_cast<typename Domains::domain_type>(meta.descriptor.domain), __runtime.data()[tensor.value].pointer,
                         source_domain, source, bytes);
  }

  [[nodiscard]] compute_status
  copy_from(tensor_id tensor, const void *source, usize bytes) noexcept
  {
    if constexpr ( requires { Domains::host; } )
      return copy_from(tensor, static_cast<typename Domains::domain_type>(Domains::host), source, bytes);
    else
      return copy_from(tensor, typename Domains::domain_type{}, source, bytes);
  }

  [[nodiscard]] void *
  data(tensor_id tensor) noexcept
  {
    return __valid_tensor(tensor) && __runtime.data()[tensor.value].valid ? __runtime.data()[tensor.value].pointer : nullptr;
  }

  [[nodiscard]] const void *
  data(tensor_id tensor) const noexcept
  {
    return __valid_tensor(tensor) && __runtime.data()[tensor.value].valid ? __runtime.data()[tensor.value].pointer : nullptr;
  }

  [[nodiscard]] shared_tensor_type
  export_tensor(tensor_id tensor) const noexcept
  {
    if ( __executing || !__valid_tensor(tensor) || !__runtime.data()[tensor.value].valid ) return {};
    const auto &meta = __plan->__tensors.data()[tensor.value];
    const auto &root_meta = __plan->__tensors.data()[meta.root.value];
    const auto &runtime = __runtime.data()[tensor.value];
    if ( runtime.borrowed || !runtime.storage || (!meta.retained && !root_meta.state) ) return {};
    if ( meta.descriptor.byte_size == 0 ) return { runtime.storage, meta.descriptor, 0 };
    const auto *base = reinterpret_cast<const byte *>(runtime.storage.data());
    const auto *pointer = reinterpret_cast<const byte *>(runtime.pointer);
    if ( pointer < base || static_cast<usize>(pointer - base) > runtime.storage.size() ) return {};
    const usize offset = static_cast<usize>(pointer - base);
    const usize origin = __logical_origin(meta.descriptor, meta.minimum_offset);
    if ( origin > offset || meta.descriptor.byte_size > runtime.storage.size() - (offset - origin) ) return {};
    return { runtime.storage, meta.descriptor, offset };
  }

  void
  reset() noexcept
  {
    if ( __executing ) return;
    for ( usize tensor = 0; tensor < __plan->__tensors.size(); ++tensor ) {
      const auto &meta = __plan->__tensors.data()[tensor];
      if ( !meta.external && !meta.state ) __runtime.data()[tensor].valid = false;
    }
  }

  void
  hard_reset() noexcept
  {
    if ( __executing ) return;
    for ( usize slot = 0; slot < __slot_storage.size(); ++slot )
      if ( __slot_storage.data()[slot].size() )
        micron::memset(__slot_storage.data()[slot].data(), byte(0), __slot_storage.data()[slot].size());
    for ( usize tensor = 0; tensor < __plan->__tensors.size(); ++tensor ) {
      const auto &meta = __plan->__tensors.data()[tensor];
      auto &runtime = __runtime.data()[tensor];
      if ( meta.root.value != tensor ) continue;
      if ( meta.external ) {
        runtime = {};
      } else if ( meta.slot != micron::numeric_limits<u32>::max() ) {
        runtime.storage = __slot_storage.data()[meta.slot];
        runtime.pointer = runtime.storage.data()
                              ? reinterpret_cast<byte *>(runtime.storage.data()) + __logical_origin(meta.descriptor, meta.minimum_offset)
                              : nullptr;
        runtime.borrowed = false;
        runtime.valid = meta.state;
      }
    }
    __refresh_runtime_pointers();
    reset();
  }

  template<typename Executor>
  [[nodiscard]] compute_result
  execute(Executor &executor) noexcept
    requires requires(Executor value, void (*function)(void *) noexcept, void *argument) {
      { value.submit(function, argument) } -> micron::convertible_to<bool>;
    }
  {
    static_assert(noexcept(executor.submit(static_cast<void (*)(void *) noexcept>(nullptr), nullptr)),
                  "compute executors must provide a noexcept submit function");
    if ( __executing ) return { compute_status::invalid_plan, operation_id::invalid() };
    if ( !__bindings_ready() ) return { compute_status::missing_binding, operation_id::invalid() };
    for ( const auto &slot : __plan->__slots )
      if ( Domains::synchronize(slot.domain) != compute_status::ok ) return { compute_status::domain_failure, operation_id::invalid() };
    __prepare_execution();
    if ( __plan->__operations.empty() ) return {};
    __executor = micron::addressof(executor);
    __submit = &__submit_adapter<Executor>;
    __executing = true;
    for ( usize operation = 0; operation < __plan->__operations.size(); ++operation )
      if ( __plan->__operations.data()[operation].dependency_count == 0 ) __schedule(static_cast<u32>(operation));
    while ( __remaining.get(micron::memory_order_acquire) != 0 ) {
      const u32 expected = __remaining.get(micron::memory_order_relaxed);
      if ( expected != 0 ) micron::wait_futex(__remaining.ptr(), expected);
    }
    __executing = false;
    __executor = nullptr;
    __submit = nullptr;
    const u32 failure = __earliest_failure.get(micron::memory_order_acquire);
    if ( failure == operation_id::invalid_value() ) return {};
    return { __operation_status.data()[failure], operation_id{ failure } };
  }

  [[nodiscard]] compute_result
  execute() noexcept
  {
    inline_executor executor;
    return execute(executor);
  }
};

template<typename Ops, domain_provider Domains>
void
compute_session<Ops, Domains>::__schedule(u32 operation) noexcept
{
  if ( !__submit || !__submit(__executor, &__job_entry, micron::addressof(__jobs.data()[operation])) )
    __complete(operation, compute_status::executor_failure, false);
}

};      // namespace micron::math::compute

namespace micron::math::compute
{

template<typename Ops>
concept __compute_operation_registry
    = requires(typename Ops::operation_type operation, typename Ops::dtype_type dtype, typename Ops::context_type context,
               micron::raw_slice<const tensor_descriptor<typename Ops::dtype_type>> input_descriptors,
               micron::raw_slice<tensor_descriptor<typename Ops::dtype_type>> output_descriptors,
               micron::raw_slice<const tensor_view<typename Ops::dtype_type>> input_views,
               micron::raw_slice<tensor_view<typename Ops::dtype_type>> output_views) {
        typename Ops::operation_type;
        typename Ops::dtype_type;
        typename Ops::context_type;
        { Ops::dtype_size(dtype) } -> micron::convertible_to<usize>;
        { Ops::output_count(operation) } -> micron::convertible_to<usize>;
        { Ops::infer(operation, input_descriptors, output_descriptors) } -> micron::same_as<compute_status>;
        { Ops::validate(operation, input_descriptors, output_descriptors) } -> micron::same_as<compute_status>;
        { Ops::execute(operation, context, input_views, output_views) } noexcept -> micron::same_as<compute_status>;
      };

namespace __impl
{
template<typename Dtype> struct __compute_tensor_meta;
template<typename Operation> struct __compute_operation_meta;
template<typename Domain> struct __compute_slot_meta;
template<typename Dtype> struct __builder_tensor;
template<typename Operation> struct __builder_operation;
void __compute_plan_error(const char *message);
template<typename Ops, domain_provider Domains>
bool __normalize_descriptor(tensor_descriptor<typename Ops::dtype_type> &, ssize_t &, ssize_t &) noexcept;
void __add_dependency(__compute_vec<__compute_vec<u32>> &, usize, usize);
};      // namespace __impl

template<typename Ops, domain_provider Domains> class compute_graph
{
  static_assert(__compute_operation_registry<Ops>, "compute_graph requires a compile-time operation registry");

public:
  using operation_type = typename Ops::operation_type;
  using dtype_type = typename Ops::dtype_type;
  using descriptor_type = tensor_descriptor<dtype_type>;
  using plan_type = compute_plan<Ops, Domains>;

private:
  using builder_tensor = __impl::__builder_tensor<dtype_type>;
  using builder_operation = __impl::__builder_operation<operation_type>;
  __impl::__compute_vec<builder_tensor> __tensors;
  __impl::__compute_vec<builder_operation> __operations;

  [[nodiscard]] bool
  __valid_tensor(tensor_id tensor) const noexcept
  {
    return tensor.valid() && tensor.value < __tensors.size();
  }

public:
  compute_graph() = default;
  compute_graph(const compute_graph &) = delete;
  compute_graph &operator=(const compute_graph &) = delete;
  compute_graph(compute_graph &&) noexcept = default;
  compute_graph &operator=(compute_graph &&) noexcept = default;

  [[nodiscard]] tensor_id
  add_input(const descriptor_type &descriptor)
  {
    if ( __tensors.size() >= tensor_id::invalid_value() ) __impl::__compute_plan_error("compute_graph: tensor id overflow");
    const tensor_id id{ static_cast<u32>(__tensors.size()) };
    __tensors.push_back({ descriptor, operation_id::invalid(), true, false, false });
    return id;
  }

  [[nodiscard]] tensor_id
  add_state(const descriptor_type &descriptor)
  {
    if ( descriptor.access != tensor_access::read_write )
      __impl::__compute_plan_error("compute_graph: persistent state must be read-write");
    if ( __tensors.size() >= tensor_id::invalid_value() ) __impl::__compute_plan_error("compute_graph: tensor id overflow");
    const tensor_id id{ static_cast<u32>(__tensors.size()) };
    __tensors.push_back({ descriptor, operation_id::invalid(), false, true, true });
    return id;
  }

  [[nodiscard]] __impl::__compute_vec<tensor_id>
  add_operation(operation_type operation, micron::raw_slice<const tensor_id> inputs, usize output_count)
  {
    if ( __operations.size() >= operation_id::invalid_value() ) __impl::__compute_plan_error("compute_graph: operation id overflow");
    if ( output_count > tensor_id::invalid_value() - __tensors.size() ) __impl::__compute_plan_error("compute_graph: tensor id overflow");
    for ( auto input : inputs )
      if ( !__valid_tensor(input) ) __impl::__compute_plan_error("compute_graph: invalid operation input");
    const operation_id producer{ static_cast<u32>(__operations.size()) };
    builder_operation node{ micron::move(operation), {}, {} };
    node.inputs.reserve(inputs.size());
    for ( auto input : inputs ) node.inputs.push_back(input);
    node.outputs.reserve(output_count);
    for ( usize output = 0; output < output_count; ++output ) {
      const tensor_id id{ static_cast<u32>(__tensors.size()) };
      __tensors.push_back({ descriptor_type{}, producer, false, false, false });
      node.outputs.push_back(id);
    }
    auto result = node.outputs;
    __operations.push_back(micron::move(node));
    return result;
  }

  [[nodiscard]] __impl::__compute_vec<tensor_id>
  add_operation(operation_type operation, micron::raw_slice<const tensor_id> inputs)
  {
    const usize outputs = static_cast<usize>(Ops::output_count(operation));
    return add_operation(micron::move(operation), inputs, outputs);
  }

  [[nodiscard]] __impl::__compute_vec<tensor_id>
  add(operation_type operation, micron::raw_slice<const tensor_id> inputs)
  {
    return add_operation(micron::move(operation), inputs);
  }

  [[nodiscard]] tensor_id
  add_one(operation_type operation, micron::raw_slice<const tensor_id> inputs)
  {
    auto outputs = add_operation(micron::move(operation), inputs);
    if ( outputs.size() != 1 ) __impl::__compute_plan_error("compute_graph: add_one operation does not have one output");
    return outputs.data()[0];
  }

  void
  set_descriptor(tensor_id tensor, const descriptor_type &descriptor)
  {
    if ( !__valid_tensor(tensor) ) __impl::__compute_plan_error("compute_graph: invalid tensor descriptor target");
    __tensors.data()[tensor.value].descriptor = descriptor;
  }

  void
  retain(tensor_id tensor)
  {
    if ( !__valid_tensor(tensor) ) __impl::__compute_plan_error("compute_graph: invalid retained tensor");
    __tensors.data()[tensor.value].retained = true;
  }

  [[nodiscard]] plan_type
  finalize()
  {
    using operation_meta = __impl::__compute_operation_meta<operation_type>;
    using slot_meta = __impl::__compute_slot_meta<typename Domains::domain_type>;

    plan_type plan;
    plan.__tensors.resize(__tensors.size());
    for ( usize tensor = 0; tensor < __tensors.size(); ++tensor ) {
      const auto &source = __tensors.data()[tensor];
      auto &target = plan.__tensors.data()[tensor];
      target.descriptor = source.descriptor;
      target.producer = source.producer;
      target.root = tensor_id{ static_cast<u32>(tensor) };
      target.external = source.external;
      target.state = source.state;
      target.retained = source.retained;
      if ( !source.producer.valid()
           && !__impl::__normalize_descriptor<Ops, Domains>(target.descriptor, target.minimum_offset, target.maximum_offset) )
        __impl::__compute_plan_error("compute_graph: invalid input or state tensor descriptor");
    }

    __impl::__compute_vec<__impl::__compute_vec<u32>> consumers(__tensors.size());
    __impl::__compute_vec<__impl::__compute_vec<u32>> dependencies(__operations.size());
    __impl::__compute_vec<alias_declaration> aliases;
    __impl::__compute_vec<descriptor_type> input_descriptors;
    __impl::__compute_vec<descriptor_type> output_descriptors;
    plan.__operations.reserve(__operations.size());

    for ( usize operation = 0; operation < __operations.size(); ++operation ) {
      auto &builder = __operations.data()[operation];
      input_descriptors.clear();
      output_descriptors.clear();
      aliases.clear();
      input_descriptors.reserve(builder.inputs.size());
      output_descriptors.reserve(builder.outputs.size());
      aliases.resize(builder.outputs.size(), alias_declaration{});
      for ( auto input : builder.inputs ) {
        input_descriptors.push_back(plan.__tensors.data()[input.value].descriptor);
        consumers.data()[input.value].push_back(static_cast<u32>(operation));
        const auto producer = plan.__tensors.data()[input.value].producer;
        if ( producer.valid() ) __impl::__add_dependency(dependencies, operation, producer.value);
      }
      for ( auto output : builder.outputs ) output_descriptors.push_back(plan.__tensors.data()[output.value].descriptor);

      const auto input_slice = micron::raw_slice<const descriptor_type>(input_descriptors.data(), input_descriptors.size());
      auto output_slice = micron::raw_slice<descriptor_type>(output_descriptors.data(), output_descriptors.size());
      if ( Ops::infer(builder.operation, input_slice, output_slice) != compute_status::ok )
        __impl::__compute_plan_error("compute_graph: operation shape/type inference failed");
      for ( usize output = 0; output < builder.outputs.size(); ++output ) {
        auto &meta = plan.__tensors.data()[builder.outputs.data()[output].value];
        meta.descriptor = output_descriptors.data()[output];
        if ( !__impl::__normalize_descriptor<Ops, Domains>(meta.descriptor, meta.minimum_offset, meta.maximum_offset) )
          __impl::__compute_plan_error("compute_graph: invalid inferred output descriptor");
        output_descriptors.data()[output] = meta.descriptor;
      }
      if ( Ops::validate(builder.operation, input_slice,
                         micron::raw_slice<descriptor_type>(output_descriptors.data(), output_descriptors.size()))
           != compute_status::ok )
        __impl::__compute_plan_error("compute_graph: operation validation failed");
      if constexpr ( requires {
                       {
                         Ops::aliases(builder.operation, input_slice,
                                      micron::raw_slice<const descriptor_type>(output_descriptors.data(), output_descriptors.size()),
                                      micron::raw_slice<alias_declaration>(aliases.data(), aliases.size()))
                       } -> micron::same_as<compute_status>;
                     } ) {
        if ( Ops::aliases(builder.operation, input_slice,
                          micron::raw_slice<const descriptor_type>(output_descriptors.data(), output_descriptors.size()),
                          micron::raw_slice<alias_declaration>(aliases.data(), aliases.size()))
             != compute_status::ok )
          __impl::__compute_plan_error("compute_graph: operation alias validation failed");
      }

      const usize input_begin = plan.__inputs.size();
      const usize output_begin = plan.__outputs.size();
      for ( auto input : builder.inputs ) plan.__inputs.push_back(input);
      for ( usize output = 0; output < builder.outputs.size(); ++output ) {
        const auto id = builder.outputs.data()[output];
        auto &meta = plan.__tensors.data()[id.value];
        const auto declaration = aliases.data()[output];
        if ( declaration.mode != alias_mode::none ) {
          if ( declaration.input_index >= builder.inputs.size()
               || declaration.offset_bytes > static_cast<usize>(micron::numeric_limits<ssize_t>::max()) )
            __impl::__compute_plan_error("compute_graph: invalid alias declaration");
          const auto input_id = builder.inputs.data()[declaration.input_index];
          const auto &input = plan.__tensors.data()[input_id.value];
          ssize_t alias_min{};
          ssize_t alias_max{};
          const ssize_t offset = static_cast<ssize_t>(declaration.offset_bytes);
          if ( __builtin_add_overflow(offset, meta.minimum_offset, &alias_min)
               || __builtin_add_overflow(offset, meta.maximum_offset, &alias_max) || alias_min < input.minimum_offset
               || alias_max > input.maximum_offset )
            __impl::__compute_plan_error("compute_graph: alias view exceeds its input storage");
          if ( declaration.offset_bytes % meta.descriptor.alignment != 0 )
            __impl::__compute_plan_error("compute_graph: alias offset violates output alignment");
          if ( declaration.mode == alias_mode::destructive_in_place
               && (input.descriptor.access != tensor_access::read_write || meta.descriptor.access != tensor_access::read_write) )
            __impl::__compute_plan_error("compute_graph: destructive alias requires read-write tensors");
          if ( declaration.mode == alias_mode::read_only_view ) meta.descriptor.access = tensor_access::read_only;
          meta.alias = declaration.mode;
          meta.alias_input = declaration.input_index;
          meta.root = input.root;
          if ( __builtin_add_overflow(input.root_offset, declaration.offset_bytes, &meta.root_offset) )
            __impl::__compute_plan_error("compute_graph: alias offset overflow");
        }
        plan.__outputs.push_back(id);
      }
      plan.__operations.push_back(operation_meta{ micron::move(builder.operation), input_begin, builder.inputs.size(), output_begin,
                                                  builder.outputs.size(), 0, 0, 0 });
    }

    __impl::__compute_vec<__impl::__compute_vec<u32>> root_consumers(__tensors.size());
    __impl::__compute_vec<usize> root_last(__tensors.size(), usize(0));
    __impl::__compute_vec<u8> root_retained(__tensors.size(), u8(0));
    for ( usize tensor = 0; tensor < plan.__tensors.size(); ++tensor ) {
      auto &meta = plan.__tensors.data()[tensor];
      const usize root = meta.root.value;
      if ( meta.retained || meta.state ) root_retained.data()[root] = 1;
      if ( meta.producer.valid() && root_last.data()[root] < meta.producer.value ) root_last.data()[root] = meta.producer.value;
      for ( u32 consumer : consumers.data()[tensor] ) {
        bool duplicate = false;
        for ( u32 existing : root_consumers.data()[root] ) duplicate |= existing == consumer;
        if ( !duplicate ) root_consumers.data()[root].push_back(consumer);
        if ( root_last.data()[root] < consumer ) root_last.data()[root] = consumer;
      }
    }

    for ( usize operation = 0; operation < plan.__operations.size(); ++operation ) {
      const auto &node = plan.__operations.data()[operation];
      for ( usize output = 0; output < node.output_count; ++output ) {
        const auto output_id = plan.__outputs.data()[node.output_begin + output];
        const auto &output_meta = plan.__tensors.data()[output_id.value];
        if ( output_meta.alias != alias_mode::destructive_in_place ) continue;
        const auto input_id = plan.__inputs.data()[node.input_begin + output_meta.alias_input];
        const usize root = plan.__tensors.data()[input_id.value].root.value;
        for ( u32 consumer : root_consumers.data()[root] ) {
          if ( consumer == operation ) continue;
          __impl::__compute_vec<u8> seen(plan.__operations.size(), u8(0));
          __impl::__compute_vec<u32> stack;
          stack.push_back(consumer);
          bool follows_write = false;
          while ( !stack.empty() && !follows_write ) {
            const u32 current = stack.data()[stack.size() - 1];
            stack.pop_back();
            if ( current == operation ) {
              follows_write = true;
              break;
            }
            if ( seen.data()[current] ) continue;
            seen.data()[current] = 1;
            for ( u32 dependency : dependencies.data()[current] ) stack.push_back(dependency);
          }
          if ( !follows_write ) __impl::__add_dependency(dependencies, operation, consumer);
        }
      }
    }

    for ( usize tensor = 0; tensor < plan.__tensors.size(); ++tensor ) {
      auto &meta = plan.__tensors.data()[tensor];
      if ( meta.root.value != tensor || meta.external ) continue;
      const bool dedicated = meta.state || root_retained.data()[tensor] != 0;
      const usize producer = meta.producer.valid() ? meta.producer.value : 0;
      const usize available = root_last.data()[tensor] < producer ? producer : root_last.data()[tensor];
      u32 selected = micron::numeric_limits<u32>::max();
      if ( !dedicated && meta.producer.valid() ) {
        for ( usize slot = 0; slot < plan.__slots.size(); ++slot ) {
          auto &candidate = plan.__slots.data()[slot];
          if ( candidate.dedicated || candidate.domain != static_cast<typename Domains::domain_type>(meta.descriptor.domain)
               || candidate.available_after >= producer )
            continue;
          selected = static_cast<u32>(slot);
          const usize previous_root = candidate.last_root.value;
          for ( u32 consumer : root_consumers.data()[previous_root] ) __impl::__add_dependency(dependencies, producer, consumer);
          const auto previous_producer = plan.__tensors.data()[previous_root].producer;
          if ( root_consumers.data()[previous_root].empty() && previous_producer.valid() )
            __impl::__add_dependency(dependencies, producer, previous_producer.value);
          if ( candidate.bytes < meta.descriptor.byte_size ) candidate.bytes = meta.descriptor.byte_size;
          if ( candidate.alignment < meta.descriptor.alignment ) candidate.alignment = meta.descriptor.alignment;
          candidate.available_after = available;
          candidate.last_root = tensor_id{ static_cast<u32>(tensor) };
          break;
        }
      }
      if ( selected == micron::numeric_limits<u32>::max() ) {
        if ( plan.__slots.size() >= micron::numeric_limits<u32>::max() )
          __impl::__compute_plan_error("compute_graph: arena slot id overflow");
        selected = static_cast<u32>(plan.__slots.size());
        plan.__slots.push_back(slot_meta{ static_cast<typename Domains::domain_type>(meta.descriptor.domain), meta.descriptor.byte_size,
                                          meta.descriptor.alignment, dedicated, tensor_id{ static_cast<u32>(tensor) }, available });
      }
      meta.slot = selected;
    }
    for ( usize tensor = 0; tensor < plan.__tensors.size(); ++tensor ) {
      auto &meta = plan.__tensors.data()[tensor];
      meta.retained = meta.retained || root_retained.data()[meta.root.value] != 0;
      if ( meta.root.value != tensor ) meta.slot = plan.__tensors.data()[meta.root.value].slot;
    }

    __impl::__compute_vec<__impl::__compute_vec<u32>> dependents(plan.__operations.size());
    __impl::__compute_vec<u32> indegree(plan.__operations.size(), u32(0));
    for ( usize operation = 0; operation < dependencies.size(); ++operation ) {
      indegree.data()[operation] = static_cast<u32>(dependencies.data()[operation].size());
      for ( u32 dependency : dependencies.data()[operation] ) dependents.data()[dependency].push_back(static_cast<u32>(operation));
    }
    __impl::__compute_vec<u32> queue;
    for ( usize operation = 0; operation < indegree.size(); ++operation )
      if ( indegree.data()[operation] == 0 ) queue.push_back(static_cast<u32>(operation));
    usize visited = 0;
    for ( usize head = 0; head < queue.size(); ++head ) {
      const u32 operation = queue.data()[head];
      ++visited;
      for ( u32 dependent : dependents.data()[operation] )
        if ( --indegree.data()[dependent] == 0 ) queue.push_back(dependent);
    }
    if ( visited != plan.__operations.size() ) __impl::__compute_plan_error("compute_graph: data or memory anti-dependencies form a cycle");

    for ( usize operation = 0; operation < plan.__operations.size(); ++operation ) {
      auto &meta = plan.__operations.data()[operation];
      meta.dependency_count = static_cast<u32>(dependencies.data()[operation].size());
      meta.dependent_begin = plan.__dependents.size();
      meta.dependent_count = dependents.data()[operation].size();
      for ( u32 dependent : dependents.data()[operation] ) plan.__dependents.push_back(dependent);
    }
    for ( const auto &slot : plan.__slots ) {
      if ( __builtin_add_overflow(plan.__peak_bytes, slot.bytes, &plan.__peak_bytes) )
        __impl::__compute_plan_error("compute_graph: arena peak byte count overflow");
    }
    return plan;
  }
};

};      // namespace micron::math::compute

namespace micron::math::compute
{

template<typename Ops>
concept operation_registry = __compute_operation_registry<Ops>;

namespace __impl
{

inline void
__compute_plan_error(const char *message)
{
  micron::exc<micron::except::invalid_argument>(message);
}

inline bool
__power_of_two(usize value) noexcept
{
  return value != 0 && (value & (value - 1)) == 0;
}

template<typename Dtype>
[[nodiscard]] bool
__descriptor_compatible(const tensor_descriptor<Dtype> &expected, const tensor_descriptor<Dtype> &actual) noexcept
{
  if ( expected.dtype != actual.dtype || expected.domain != actual.domain || expected.byte_size != actual.byte_size
       || expected.alignment != actual.alignment || expected.dimensions.size() != actual.dimensions.size()
       || expected.strides.size() != actual.strides.size() )
    return false;
  for ( usize i = 0; i < expected.dimensions.size(); ++i )
    if ( expected.dimensions.data()[i] != actual.dimensions.data()[i] || expected.strides.data()[i] != actual.strides.data()[i] )
      return false;
  return true;
}

template<typename Ops, domain_provider Domains>
[[nodiscard]] bool
__normalize_descriptor(tensor_descriptor<typename Ops::dtype_type> &descriptor, ssize_t &minimum, ssize_t &maximum) noexcept
{
  static_assert(operation_registry<Ops>);
  using domain_type = typename Domains::domain_type;
  const domain_type domain = static_cast<domain_type>(descriptor.domain);
  if ( !Domains::valid(domain) ) return false;
  const usize element = static_cast<usize>(Ops::dtype_size(descriptor.dtype));
  if ( element == 0 || element > static_cast<usize>(micron::numeric_limits<ssize_t>::max()) ) return false;
  usize alignment = descriptor.alignment;
  const usize domain_alignment = Domains::minimum_alignment(domain);
  if ( alignment == 0 ) alignment = domain_alignment;
  if ( !__power_of_two(alignment) ) return false;
  descriptor.alignment = alignment;

  if ( descriptor.strides.empty() ) {
    descriptor.strides.resize(descriptor.dimensions.size(), ssize_t(0));
    usize stride = element;
    for ( usize i = descriptor.dimensions.size(); i != 0; --i ) {
      if ( stride > static_cast<usize>(micron::numeric_limits<ssize_t>::max()) ) return false;
      descriptor.strides.data()[i - 1] = static_cast<ssize_t>(stride);
      if ( descriptor.dimensions.data()[i - 1] != 0 && __builtin_mul_overflow(stride, descriptor.dimensions.data()[i - 1], &stride) )
        return false;
    }
  } else if ( descriptor.strides.size() != descriptor.dimensions.size() ) {
    return false;
  }

  minimum = 0;
  maximum = 0;
  bool empty = false;
  for ( usize i = 0; i < descriptor.dimensions.size(); ++i ) {
    const usize dimension = descriptor.dimensions.data()[i];
    if ( dimension == 0 ) {
      empty = true;
      continue;
    }
    if ( dimension - 1 > static_cast<usize>(micron::numeric_limits<ssize_t>::max()) ) return false;
    ssize_t delta{};
    if ( __builtin_mul_overflow(static_cast<ssize_t>(dimension - 1), descriptor.strides.data()[i], &delta) ) return false;
    if ( delta < 0 ) {
      if ( __builtin_add_overflow(minimum, delta, &minimum) ) return false;
    } else if ( __builtin_add_overflow(maximum, delta, &maximum) ) {
      return false;
    }
  }
  usize extent = 0;
  if ( !empty ) {
    ssize_t span{};
    if ( __builtin_sub_overflow(maximum, minimum, &span) || __builtin_add_overflow(span, static_cast<ssize_t>(element), &span) || span < 0 )
      return false;
    extent = static_cast<usize>(span);
  }
  if ( descriptor.byte_size != 0 && descriptor.byte_size != extent ) return false;
  descriptor.byte_size = extent;
  return true;
}

template<typename Dtype> struct __compute_tensor_meta {
  tensor_descriptor<Dtype> descriptor;
  operation_id producer{};
  tensor_id root{};
  usize root_offset{};
  ssize_t minimum_offset{};
  ssize_t maximum_offset{};
  u32 slot{ micron::numeric_limits<u32>::max() };
  alias_mode alias{ alias_mode::none };
  usize alias_input{};
  bool external{};
  bool state{};
  bool retained{};
};

template<typename Operation> struct __compute_operation_meta {
  Operation operation;
  usize input_begin{};
  usize input_count{};
  usize output_begin{};
  usize output_count{};
  usize dependent_begin{};
  usize dependent_count{};
  u32 dependency_count{};
};

template<typename Domain> struct __compute_slot_meta {
  Domain domain{};
  usize bytes{};
  usize alignment{};
  bool dedicated{};
  tensor_id last_root{};
  usize available_after{};
};

template<typename Dtype> struct __builder_tensor {
  tensor_descriptor<Dtype> descriptor;
  operation_id producer{};
  bool external{};
  bool state{};
  bool retained{};
};

template<typename Operation> struct __builder_operation {
  Operation operation;
  __compute_vec<tensor_id> inputs;
  __compute_vec<tensor_id> outputs;
};

inline void
__add_dependency(__compute_vec<__compute_vec<u32>> &dependencies, usize operation, usize dependency)
{
  if ( operation == dependency ) return;
  auto &row = dependencies.data()[operation];
  for ( u32 existing : row )
    if ( existing == dependency ) return;
  row.push_back(static_cast<u32>(dependency));
}

};      // namespace __impl

template<typename Ops, domain_provider Domains> class compute_plan
{
  static_assert(operation_registry<Ops>, "compute_plan requires a compile-time operation registry");
  using dtype_type = typename Ops::dtype_type;
  using operation_type = typename Ops::operation_type;
  using domain_type = typename Domains::domain_type;
  using tensor_meta = __impl::__compute_tensor_meta<dtype_type>;
  using operation_meta = __impl::__compute_operation_meta<operation_type>;
  using slot_meta = __impl::__compute_slot_meta<domain_type>;

  __impl::__compute_vec<tensor_meta> __tensors;
  __impl::__compute_vec<operation_meta> __operations;
  __impl::__compute_vec<tensor_id> __inputs;
  __impl::__compute_vec<tensor_id> __outputs;
  __impl::__compute_vec<u32> __dependents;
  __impl::__compute_vec<slot_meta> __slots;
  usize __peak_bytes{};

  friend class compute_graph<Ops, Domains>;
  friend class compute_session<Ops, Domains>;

public:
  using registry_type = Ops;
  using domains_type = Domains;
  using descriptor_type = tensor_descriptor<dtype_type>;

  compute_plan() = default;
  compute_plan(const compute_plan &) = delete;
  compute_plan &operator=(const compute_plan &) = delete;
  compute_plan(compute_plan &&) noexcept = default;
  compute_plan &operator=(compute_plan &&) noexcept = default;

  [[nodiscard]] usize
  tensors_count() const noexcept
  {
    return __tensors.size();
  }

  [[nodiscard]] usize
  operations_count() const noexcept
  {
    return __operations.size();
  }

  [[nodiscard]] usize
  slots_count() const noexcept
  {
    return __slots.size();
  }

  [[nodiscard]] usize
  peak_bytes() const noexcept
  {
    return __peak_bytes;
  }

  [[nodiscard]] const descriptor_type *
  descriptor(tensor_id tensor) const noexcept
  {
    return tensor.valid() && tensor.value < __tensors.size() ? micron::addressof(__tensors.data()[tensor.value].descriptor) : nullptr;
  }

  [[nodiscard]] bool
  retained(tensor_id tensor) const noexcept
  {
    return tensor.valid() && tensor.value < __tensors.size() && __tensors.data()[tensor.value].retained;
  }

  [[nodiscard]] bool
  persistent(tensor_id tensor) const noexcept
  {
    return tensor.valid() && tensor.value < __tensors.size() && __tensors.data()[tensor.value].state;
  }
};

};      // namespace micron::math::compute
