//  Copyright (c) 2026- David Lucius Severus
//
//  Distributed under the Boost Software License, Version 1.0.
//  See accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt

#include "../../src/maps.hpp"

int
main()
{
  const u64 key = 7;
  const u64 h = micron::hash<micron::hash64_t>(key);
  u64 acc = 0;

  micron::robin_map<u64, u64> robin(64);
  robin.insert_hash(h, key, u64{ 1 });
  acc += *robin.find_hash(h, key);

  micron::heap_swiss_map<u64, u64> heap_swiss(64);
  heap_swiss.insert_hash(h, key, u64{ 2 });
  acc += *heap_swiss.find_hash(h, key);

  micron::stack_swiss_map<u64, u64, 64> stack_swiss;
  stack_swiss.insert_hash(h, key, u64{ 3 });
  acc += *stack_swiss.find_hash(h, key);

  micron::hopscotch_map<u64, u64> hop;
  hop.insert_asis(h, u64{ 4 });
  acc += *hop.find_hash(h);

  micron::btree_map<u64, u64> btree(16);
  btree.insert_hash(h, key, u64{ 5 });
  acc += *btree.find_hash(h, key);

  micron::rb_map<u64, u64> rb;
  rb.insert_hash(h, u64{ key }, u64{ 6 });
  acc += *rb.find_hash(h, key);

  micron::conmap<u64, u64> concurrent(4096);
  concurrent.insert_hash(h, key, u64{ 7 });
  u64 out = 0;
  concurrent.find_hash(h, key, out);
  acc += out;

  micron::immutable_map<u64, u64> immutable;
  auto immutable_next = immutable.insert(key, 8);
  acc += *immutable_next.find(key);

  micron::immutable_table<u64, u64> table;
  auto table_next = table.insert(key, 9);
  acc += *table_next.find(key);

  micron::pmap<u64, u64> persistent;
  auto persistent_next = persistent.insert(key, 10);
  acc += *persistent_next.find(key);

  return static_cast<int>(acc & 0x7fu);
}
