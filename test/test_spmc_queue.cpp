// test_spmc_queue.cpp

#include "spmc_queue.h"
#include <doctest/doctest.h>
#include <memory>

TEST_CASE("Test SpmcQueue") {
  SpmcQueue<int*, 4> queue;

  // Test capacity
  CHECK(queue.capacity() == 15);

  // Test empty
  CHECK(queue.empty());

  // Test push and size
  int* item1 = new int(1);
  int* item2 = new int(2);
  CHECK(queue.try_push(item1));
  CHECK(queue.size() == 1);
  CHECK(queue.try_push(item2));
  CHECK(queue.size() == 2);

  // Test pop
  int* poppedItem = queue.pop();
  CHECK(poppedItem == item1);
  CHECK(queue.size() == 1);

  // Test steal
  int* stolenItem = queue.steal();
  CHECK(stolenItem == item2);
  CHECK(queue.empty());

  // Test push with on_full callback
  int* item3 = new int(3);
  int* item4 = new int(4);
  for (int i = 0; i < 15; ++i) {
    queue.push(new int(i), []() { CHECK(false); });
  }
  queue.push(item3, []() { CHECK(true); });

  // Test steal after pushing multiple items
  for (int i = 0; i < 15; ++i) {
    int* stolen = queue.steal();
    CHECK(stolen != nullptr);
  }

  // Clean up
  delete item1;
  delete item2;
  delete item3;
  delete item4;
}
