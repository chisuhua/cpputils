#include "String.h"
#include "Queue.h"  // 假设你有一个 Queue 类的头文件
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <thread>
#include <vector>
//#include "doctest.h"


TEST_CASE("SPSCQueue Tests") {
    const size_t capacity = 16;
    SPSCQueue<int, capacity> q;

    SUBCASE("Basic Operations") {
        REQUIRE(q.enqueue(1));
        REQUIRE(q.enqueue(2));
        REQUIRE(q.enqueue(3));

        int value;
        REQUIRE(q.dequeue(value));
        REQUIRE(value == 1);
        REQUIRE(q.dequeue(value));
        REQUIRE(value == 2);
        REQUIRE(q.dequeue(value));
        REQUIRE(value == 3);
        REQUIRE(q.empty());
    }

    SUBCASE("Empty Queue") {
        REQUIRE(q.empty());
        int value;
        REQUIRE(!q.dequeue(value));
    }

    SUBCASE("Multiple Enqueue and Dequeue") {
        for (int i = 0; i < 10; ++i) {
            REQUIRE(q.enqueue(i));
        }

        for (int i = 0; i < 10; ++i) {
            int value;
            REQUIRE(q.dequeue(value));
            REQUIRE(value == i);
        }

        REQUIRE(q.empty());
    }

    SUBCASE("Concurrent Enqueue and Dequeue") {
        std::thread producer([&q] {
            for (int i = 0; i < 100; ++i) {
                while (!q.enqueue(i)) {
                    std::this_thread::yield();
                }
            }
        });

        std::thread consumer([&q] {
            for (int i = 0; i < 100; ++i) {
                int value;
                while (!q.dequeue(value)) {
                    std::this_thread::yield();
                }
                REQUIRE(value == i);
            }
        });

        producer.join();
        consumer.join();
    }
}

TEST_CASE("SPMCQueue Tests") {
    const size_t capacity = 16;
    SPMCQueue<int, capacity> q;

    SUBCASE("Basic Operations") {
        REQUIRE(q.enqueue(1));
        REQUIRE(q.enqueue(2));
        REQUIRE(q.enqueue(3));

        int value;
        REQUIRE(q.dequeue(value));
        REQUIRE(value == 1);
        REQUIRE(q.dequeue(value));
        REQUIRE(value == 2);
        REQUIRE(q.dequeue(value));
        REQUIRE(value == 3);
        REQUIRE(q.empty());
    }

    SUBCASE("Empty Queue") {
        REQUIRE(q.empty());
        int value;
        REQUIRE(!q.dequeue(value));
    }

    SUBCASE("Multiple Enqueue and Dequeue") {
        for (int i = 0; i < 10; ++i) {
            REQUIRE(q.enqueue(i));
        }

        for (int i = 0; i < 10; ++i) {
            int value;
            REQUIRE(q.dequeue(value));
            REQUIRE(value == i);
        }

        REQUIRE(q.empty());
    }

    SUBCASE("Concurrent Enqueue and Dequeue with Multiple Consumers") {
        std::thread producer([&q] {
            for (int i = 0; i < 100; ++i) {
                while (!q.enqueue(i)) {
                    std::this_thread::yield();
                }
            }
        });

        std::vector<std::thread> consumers;
        for (int i = 0; i < 5; ++i) {
            consumers.emplace_back([&q] {
                for (int j = 0; j < 20; ++j) {
                    int value;
                    while (!q.dequeue(value)) {
                        std::this_thread::yield();
                    }
                    REQUIRE(value >= 0);
                    REQUIRE(value < 100);
                }
            });
        }

        producer.join();
        for (auto& t : consumers) {
            t.join();
        }
    }
}

TEST_CASE("Test StringPool") {
    auto pool = StringPool::getInstance();

    SUBCASE("Test intern and try_emplace") {
        auto s1 = StringPool::try_emplace("hello");
        auto s2 = StringPool::try_emplace("hello");
        auto s3 = StringPool::try_emplace("world");

        CHECK(s1.get() == s2.get());
        CHECK(s1.get() != s3.get());
        CHECK(s1->data == "hello");
        CHECK(s3->data == "world");
    }

    SUBCASE("Test isStringIntern") {
        auto s1 = StringPool::try_emplace("hello");
        CHECK(pool->isStringIntern("hello"));
        CHECK_FALSE(pool->isStringIntern("goodbye"));
    }

    SUBCASE("Test getStringByHash") {
        auto s1 = StringPool::try_emplace("hello");
        auto s2 = pool->getStringByHash(fnv1a_runtime("hello", 5));
        CHECK(s1.get() == s2.get());

        auto s3 = pool->getStringByHash(fnv1a_runtime("world", 5));
        CHECK(s3 == nullptr);
    }

    SUBCASE("Test StringRef") {
        auto s1 = StringPool::try_emplace("hello");
        StringRef ref1(s1);
        StringRef ref2("hello");
        StringRef ref3("world");

        CHECK(ref1 == ref2);
        CHECK(ref1 != ref3);
        CHECK(ref1->data == "hello");
        CHECK(ref3->data == "world");

        CHECK(ref1.length() == 5);
        CHECK(ref1[0] == 'h');
        CHECK(ref1.at(4) == 'o');
        CHECK(ref1.find("ll", 0) == 2);
        CHECK(ref1.rfind("l", 4) == 3);
        CHECK(ref1.substr(1, 3) == "ell");
    }

    SUBCASE("Test operator\"\"_hs") {
        auto s1 = "hello"_hs;
        auto s2 = "hello"_hs;
        auto s3 = "world"_hs;

        CHECK(s1.get() == s2.get());
        CHECK(s1.get() != s3.get());
        CHECK(s1->data == "hello");
        CHECK(s3->data == "world");
    }
}


//int main(int argc, char** argv) {
    //doctest::Context context;
    //context.applyCommandLine(argc, argv);
    //return context.run();
//}

