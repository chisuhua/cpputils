#ifndef CPPUTILS_SPSQUEUE_H
#define CPPUTILS_SPSQUEUE_H

#include <atomic>
#include <new>
#include <type_traits>
#include <utility>
#include <cassert>
#include <stdexcept>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <array>
#include <vector>
#include <algorithm>

// 单生产者单消费者无锁队列
// 队列在常见路径上是无锁且无等待的
// 根据 AllocationMode 决定是否动态扩展容量

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

namespace cpputils {

template<typename T, size_t INITIAL_CAPACITY = 512>
class alignas(CACHE_LINE_SIZE) SpscQueue {
public:
    using value_type = T;
    using size_type = std::size_t;

    // 枚举类型，决定是否允许动态扩展容量
    enum AllocationMode { CanAlloc, CannotAlloc };

    // 构造一个可以容纳 `INITIAL_CAPACITY` 个元素的队列
    explicit SpscQueue()
        : head_(0), tail_(0), capacity_(INITIAL_CAPACITY), buffer_(capacity_) {
        assert(INITIAL_CAPACITY > 0 && "INITIAL_CAPACITY 必须大于 0");
        assert((INITIAL_CAPACITY & (INITIAL_CAPACITY - 1)) == 0 && "INITIAL_CAPACITY 必须是 2 的幂");
    }

    SpscQueue(SpscQueue&& other) noexcept
        : buffer_(std::move(other.buffer_)), head_(other.head_.load()), tail_(other.tail_.load()), capacity_(other.capacity_) {
        other.head_.store(0);
        other.tail_.store(0);
        other.capacity_ = 0;
    }

    SpscQueue& operator=(SpscQueue&& other) noexcept {
        if (this != &other) {
            buffer_ = std::move(other.buffer_);
            head_.store(other.head_.load());
            tail_.store(other.tail_.load());
            capacity_ = other.capacity_;

            other.head_.store(0);
            other.tail_.store(0);
            other.capacity_ = 0;
        }
        return *this;
    }

    ~SpscQueue() {
        for (size_t i = 0; i < capacity_; ++i) {
            if (buffer_[i]) {
                buffer_[i].~T();
            }
        }
    }

    bool empty() const {
        return head_.load() == tail_.load();
    }

    bool full() const {
        return ((tail_.load() + 1) & (capacity_ - 1)) == head_.load();
    }

    size_type size() const {
        return (tail_.load() - head_.load()) & (capacity_ - 1);
    }

    size_type capacity() const {
        return capacity_ - 1;
    }

    bool try_enqueue(const T& element) {
        return inner_enqueue<CannotAlloc>(element);
    }

    bool try_enqueue(T&& element) {
        return inner_enqueue<CannotAlloc>(std::move(element));
    }

    template<typename... Args>
    bool try_emplace(Args&&... args) {
        return inner_enqueue<CannotAlloc>(std::forward<Args>(args)...);
    }

    bool enqueue(const T& element) {
        return inner_enqueue<CanAlloc>(element);
    }

    bool enqueue(T&& element) {
        return inner_enqueue<CanAlloc>(std::move(element));
    }

    template<typename... Args>
    bool emplace(Args&&... args) {
        return inner_enqueue<CanAlloc>(std::forward<Args>(args)...);
    }

    template<typename U>
    bool try_dequeue(U& result) {
        auto head_ = head_.load();
        auto tail_ = tail_.load();

        if (head_ != tail_) {
            std::atomic_thread_fence(std::memory_order_acquire);

            auto element = &buffer_[head_ & (capacity_ - 1)];
            result = std::move(*element);
            element->~T();

            std::atomic_thread_fence(std::memory_order_release);
            head_.store((head_ + 1) & (capacity_ - 1));
        } else {
            return false;
        }

        return true;
    }

    T* peek() const {
        auto head_ = head_.load();
        auto tail_ = tail_.load();

        if (head_ != tail_) {
            std::atomic_thread_fence(std::memory_order_acquire);

            return &buffer_[head_ & (capacity_ - 1)];
        }

        return nullptr;
    }

    bool pop() {
        auto head_ = head_.load();
        auto tail_ = tail_.load();

        if (head_ != tail_) {
            std::atomic_thread_fence(std::memory_order_acquire);

            auto element = &buffer_[head_ & (capacity_ - 1)];
            element->~T();

            std::atomic_thread_fence(std::memory_order_release);
            head_.store((head_ + 1) & (capacity_ - 1));
        } else {
            return false;
        }

        return true;
    }

private:
    template<bool canAlloc, typename... Args>
    bool inner_enqueue(Args&&... args) {
        auto head_ = head_.load();
        auto tail_ = tail_.load();

        auto nextTail = (tail_ + 1) & (capacity_ - 1);
        if (nextTail != head_) {
            std::atomic_thread_fence(std::memory_order_acquire);

            auto location = &buffer_[tail_ & (capacity_ - 1)];
            new (location) T(std::forward<Args>(args)...);

            std::atomic_thread_fence(std::memory_order_release);
            tail_.store(nextTail);
        } else if (canAlloc) {
            resize();
            return inner_enqueue<CanAlloc>(std::forward<Args>(args)...);
        } else {
            return false;
        }

        return true;
    }

    void resize() {
        size_type new_capacity = capacity_ * 2;
        std::vector<T> new_buffer(new_capacity);

        auto head_ = head_.load(std::memory_order_acquire);
        auto tail_ = tail_.load(std::memory_order_acquire);
        size_type size = (tail_ - head_) & (capacity_ - 1);

        for (size_type i = 0; i < size; ++i) {
            new_buffer[i] = std::move(buffer_[(head_ + i) & (capacity_ - 1)]);
        }

        head_.store(0, std::memory_order_release);
        tail_.store(size, std::memory_order_release);
        capacity_ = new_capacity;
        buffer_.swap(new_buffer);
    }

    std::vector<T> buffer_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
    size_t capacity_;
};

} // namespace cpputils

#endif // CPPUTILS_SPSQUEUE_H

