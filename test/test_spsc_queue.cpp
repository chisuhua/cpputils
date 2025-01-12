#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "SpscQueue.h"

TEST_CASE("Basic operations") {
    cpputils::SpscQueue<int> queue;

    CHECK(queue.empty());
    CHECK(!queue.full());
    CHECK(queue.size() == 0);

    SUBCASE("Enqueue and Dequeue") {
        queue.enqueue(1);
        CHECK(!queue.empty());
        CHECK(queue.size() == 1);

        int value;
        CHECK(queue.try_dequeue(value));
        CHECK(value == 1);
        CHECK(queue.empty());
        CHECK(queue.size() == 0);
    }

    SUBCASE("Try Enqueue and Try Dequeue") {
        CHECK(queue.try_enqueue(2));
        CHECK(queue.size() == 1);

        int value;
        CHECK(queue.try_dequeue(value));
        CHECK(value == 2);
        CHECK(queue.empty());
        CHECK(queue.size() == 0);
    }

    SUBCASE("Emplace") {
        queue.emplace(3);
        CHECK(queue.size() == 1);

        int value;
        CHECK(queue.try_dequeue(value));
        CHECK(value == 3);
        CHECK(queue.empty());
        CHECK(queue.size() == 0);
    }

    SUBCASE("Peek and Pop") {
        queue.enqueue(4);
        CHECK(queue.peek() == 4);
        CHECK(queue.size() == 1);

        CHECK(queue.pop());
        CHECK(queue.empty());
        CHECK(queue.size() == 0);
    }
}

TEST_CASE("Dynamic resizing") {
    cpputils::SpscQueue<int> queue(2);

    queue.enqueue(1);
    queue.enqueue(2);
    CHECK(queue.full());
    CHECK(queue.size() == 2);

    queue.enqueue(3);
    CHECK(!queue.full());
    CHECK(queue.size() == 3);

    int value;
    CHECK(queue.try_dequeue(value));
    CHECK(value == 1);
    CHECK(queue.size() == 2);

    CHECK(queue.try_dequeue(value));
    CHECK(value == 2);
    CHECK(queue.size() == 1);

    CHECK(queue.try_dequeue(value));
    CHECK(value == 3);
    CHECK(queue.empty());
    CHECK(queue.size() == 0);
}

TEST_CASE("Concurrent access") {
    cpputils::SpscQueue<int> queue(1024);

    std::thread producer([&queue]() {
        for (int i = 0; i < 1000; ++i) {
            queue.enqueue(i);
        }
    });

    std::thread consumer([&queue]() {
        for (int i = 0; i < 1000; ++i) {
            int value;
            while (!queue.try_dequeue(value)) {
                std::this_thread::yield();
            }
            CHECK(value == i);
        }
    });

    producer.join();
    consumer.join();
}

