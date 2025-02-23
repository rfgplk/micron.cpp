#pragma once

#include "algorithm/algorithm.hpp"
#include "algorithm/mem.hpp"
#include "heap/cpp.hpp"
#include "tags.hpp"
#include "types.hpp"
#include <exception>
#include <stdexcept>

namespace micron
{
template <typename T> struct list_node {
  T data;
  list_node<T> *next;
};
template <typename T> class list
{
  using list_node_t = list_node<T>;
  list_node_t *root;     // root node -- is a pointer (simpler to impl)
  inline void
  __impl_heap(T &&t, list_node_t **ptr)
  {
    *ptr = new list_node_t(micron::move(t), nullptr);
  }
  inline void
  __impl_heap(const T &t, list_node_t **ptr)
  {
    *ptr = new list_node_t(t, nullptr);
  }
  inline void
  deep_copy(list_node_t *dst, const list_node_t *src)
  {
    if ( src == nullptr or dst == nullptr ) {
      return;
    }
    const list_node_t *ptr = src;
    while ( ptr != nullptr or dst != nullptr ) {
      dst->data = ptr->data;
      ptr = ptr->next;
      dst = dst->next;
    }
  }

public:
  using category_type = list_tag;
  using mutability_type = mutable_tag;
  using memory_type = heap_tag;
  typedef T value_type;
  typedef T &reference;
  typedef T &ref;
  typedef const T &const_reference;
  typedef const T &const_ref;
  typedef list_node_t *pointer;
  typedef const list_node_t *const_pointer;
  typedef T *iterator;
  typedef const T *const_iterator;

  list() : root(nullptr) {}
  list(const size_t cnt) : root(nullptr)
  {
    __impl_heap(micron::move(T()), &root);
    if ( root == nullptr )
      throw std::runtime_error("micron::list() heap allocation failure");
    list_node_t *ptr = root;
    for ( size_t i = 0; i < cnt; i++ ) {
      __impl_heap(micron::move(T()), &ptr->next);
      ptr = ptr->next;
    }
  }
  list(const size_t cnt, const T &t)
  {
    __impl_heap(t, &root);
    if ( root == nullptr )
      throw std::runtime_error("micron::list() heap allocation failure");
    list_node_t *ptr = root;
    for ( size_t i = 0; i < cnt; i++ ) {
      __impl_heap(t, &ptr->next);
      ptr = ptr->next;
    }
  }
  list(const list &o) { deep_copy(root, o.root); }
  list(list &&o) : root(o.root) { o.root = nullptr; }
  list &
  operator=(const list &o)
  {
    deep_copy(&root, &o.root);
    return *this;
  }
  list &
  operator=(list &&o)
  {
    root = o.root;
    o.root.data = T();
    o.root.next = nullptr;
    return *this;
  }
  ~list()
  {
    if ( root == nullptr )
      return;
    size_t cnt = size();
    if ( cnt == 0 )
      return;
    size_t i = 0;
    list_node_t *ptr = root;
    list_node_t **ptrs = new list_node_t *[cnt + 1];

    while ( ptr != nullptr ) {
      ptrs[i++] = ptr;
      ptr->data.~T();
      ptr = ptr->next;
    };
    for ( size_t j = 0; j < i; j++ )
      delete ptrs[j];
    delete[] ptrs;
  }
  const_pointer
  iend() const
  {
    list_node_t *ptr = root;
    while ( ptr->next != nullptr )
      ptr = ptr->next;
    return ptr;
  }
  const_pointer
  ibegin() const
  {
    return root;
  }
  const_iterator
  end() const
  {
    list_node_t *ptr = root;
    while ( ptr->next != nullptr )
      ptr = ptr->next;
    return &ptr->data;
  }
  const_iterator
  begin() const
  {
    return &root->data;
  }
  void
  push_front(const T &v)
  {
    list_node_t *ptr;
    __impl_heap(v, &ptr);     // allocate ptr
    ptr->next = root;
    root = ptr;
  }
  void
  push_back(const T &v)
  {
    list_node_t *ptr;
    __impl_heap(v, &ptr);     // allocate ptr
    auto end_p = end();
    end_p->next = ptr;
  }
  const_iterator
  find(const T &srch)
  {
    auto *ptr = root;
    while ( ptr != nullptr ) {
      if ( ptr->data == srch )
        break;
      ptr = ptr->next;
    }     // ptr is found node
    return &ptr->data;     // nullptr if no hit :/
  }
  // advance by how many
  const_pointer
  next(const_pointer itr, const size_t n = 0)
  {
    if ( itr == nullptr )
      throw std::runtime_error("micron::next invalid iterator.");
    for ( size_t i = 0; i < (n + 1); i++ ) {
      if ( itr->next == nullptr )
        break;
      itr = itr->next;
    }
    return itr;
  }
  void
  erase(const_pointer itr)
  {
    if ( itr == nullptr )
      throw std::runtime_error("micron::erase invalid iterator.");
    auto *ptr = root;
    while ( ptr != nullptr ) {
      if ( ptr->next == itr )     // if parent found
        break;
      ptr = ptr->next;
    }     // ptr is now parent node
    itr->data.~T();            // call dest
    ptr->next = itr->next;     // relink
    delete itr;
  }
  void
  insert(const_pointer itr, T &&v)
  {
    if ( itr == nullptr )
      throw std::runtime_error("micron::insert invalid iterator.");
    list_node_t *ptr;
    __impl_heap(micron::move(v), &ptr);     // allocate ptr
    ptr->next = itr->next;
    itr->next = ptr;
  }
  void
  insert(const_pointer itr, const T &v)
  {
    if ( itr == nullptr )
      throw std::runtime_error("micron::insert invalid iterator.");
    list_node_t *ptr;
    __impl_heap(v, &ptr);     // allocate ptr
    ptr->next = itr->next;
    itr->next = ptr;
  }
  void
  emplace(const_pointer itr, T &&v)
  {
    if ( itr == nullptr )
      throw std::runtime_error("micron::insert invalid iterator.");
    list_node_t *ptr;
    __impl_heap(std::forward(v), &ptr);     // allocate ptr
    ptr->next = itr->next;
    itr->next = ptr;
  }
  T
  front() const
  {
    if ( root == nullptr )
      throw std::runtime_error("micron::list front() is empty");
    return root->data;
  }
  T
  back() const
  {
    if ( root == nullptr )
      throw std::runtime_error("micron::list front() is empty");
    return end()->data;
  }
  void
  merge(list &o)
  {
    if ( &o == this )
      return;
    auto *end_ptr = const_cast<pointer>(iend());
    end_ptr->next = o.root;
    o.root = nullptr;
  }
  void
  merge(list &&o)
  {
    if ( o == *this )
      return;
    auto *end_ptr = const_cast<pointer>(iend());     // dirty
    end_ptr->next = o.root;
    o.root = nullptr;
  }
  void
  splice(const_pointer pos, list &o)
  {
    if ( o == *this )
      return;
    auto *ptr = o.iend();
    ptr->next = pos->next;
    pos->next = nullptr;
  }
  bool
  empty() const
  {
    if ( root == nullptr )
      return true;
    else
      return false;
  }
  size_t
  size() const
  {
    if ( root == nullptr )
      return 0;
    size_t cnt = 0;
    auto *ptr = root->next;
    while ( ptr != nullptr ) {
      cnt++;
      ptr = ptr->next;
    }
    return cnt;
  }
  size_t
  max_size() const
  {
    return 0xFFFFFFFFFFFFFFF;
  }
};
};     // namespace micron
