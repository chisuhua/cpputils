#ifndef QUEUE_H
#define QUEUE_H

#include <vector>
#include <atomic>
#include <stdexcept>

template<typename T, size_t Capacity>
class SPMCQueue {
public:
    SPMCQueue() : mask(Capacity - 1) {
        if (Capacity & mask) {
            throw std::invalid_argument("Capacity must be a power of two.");
        }
        data.resize(Capacity);
        head.store(0, std::memory_order_relaxed);
        tail.store(0, std::memory_order_relaxed);
    }

    bool enqueue(const T& value) {
        size_t currentTail = tail.load(std::memory_order_acquire);
        while (true) {
            size_t nextTail = (currentTail + 1) & mask;
            if (nextTail == head.load(std::memory_order_acquire)) {
                return false;
            }
            data[currentTail] = value;
            if (tail.compare_exchange_weak(currentTail, nextTail, std::memory_order_release, std::memory_order_acquire)) {
                return true;
            }
        }
    }

    template<typename... Args>
    bool emplace(Args&&... args) {
        T value(std::forward<Args>(args)...);
        return enqueue(std::move(value));
    }

    bool dequeue(T& result) {
        size_t currentHead = head.load(std::memory_order_acquire);
        while (true) {
            size_t currentTail = tail.load(std::memory_order_acquire);
            if (currentHead == currentTail) {
                return false;
            }
            result = data[currentHead];
            size_t nextHead = (currentHead + 1) & mask;
            if (head.compare_exchange_weak(currentHead, nextHead, std::memory_order_release, std::memory_order_acquire)) {
                return true;
            }
        }
    }

    bool steal(T& result) {
        size_t currentHead = head.load(std::memory_order_acquire);
        while (true) {
            size_t currentTail = tail.load(std::memory_order_acquire);
            if (currentHead == currentTail) {
                return false;
            }
            result = data[currentHead];
            size_t nextHead = (currentHead + 1) & mask;
            if (head.compare_exchange_weak(currentHead, nextHead, std::memory_order_release, std::memory_order_acquire)) {
                return true;
            }
        }
    }

    bool empty() const {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }

    bool full() const {
        size_t currentTail = tail.load(std::memory_order_acquire);
        size_t nextTail = (currentTail + 1) & mask;
        return nextTail == head.load(std::memory_order_acquire);
    }

    size_t size() const {
        size_t currentHead = head.load(std::memory_order_acquire);
        size_t currentTail = tail.load(std::memory_order_acquire);
        return (currentTail - currentHead + Capacity) & mask;
    }

    size_t capacity() const {
        return Capacity;
    }

private:
    std::vector<T> data;
    std::atomic<size_t> head, tail;
    size_t mask;
};

template<typename T, size_t Capacity>
class SPSCQueue {
public:
    SPSCQueue() : mask(Capacity - 1) {
        if (Capacity & mask) {
            throw std::invalid_argument("Capacity must be a power of two.");
        }
        data.resize(Capacity);
        head.store(0, std::memory_order_relaxed);
        tail.store(0, std::memory_order_relaxed);
    }

    bool enqueue(const T& value) {
        size_t currentTail = tail.load(std::memory_order_acquire);
        while (true) {
            size_t nextTail = (currentTail + 1) & mask;
            if (nextTail == head.load(std::memory_order_acquire)) {
                return false;
            }
            data[currentTail] = value;
            if (tail.compare_exchange_weak(currentTail, nextTail, std::memory_order_release, std::memory_order_acquire)) {
                return true;
            }
        }
    }

    template<typename... Args>
    bool emplace(Args&&... args) {
        T value(std::forward<Args>(args)...);
        return enqueue(std::move(value));
    }

    bool dequeue(T& result) {
        size_t currentHead = head.load(std::memory_order_acquire);
        while (true) {
            size_t currentTail = tail.load(std::memory_order_acquire);
            if (currentHead == currentTail) {
                return false;
            }
            result = data[currentHead];
            size_t nextHead = (currentHead + 1) & mask;
            if (head.compare_exchange_weak(currentHead, nextHead, std::memory_order_release, std::memory_order_acquire)) {
                return true;
            }
        }
    }

    bool empty() const {
        return head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire);
    }

    bool full() const {
        size_t currentTail = tail.load(std::memory_order_acquire);
        size_t nextTail = (currentTail + 1) & mask;
        return nextTail == head.load(std::memory_order_acquire);
    }

    size_t size() const {
        size_t currentHead = head.load(std::memory_order_acquire);
        size_t currentTail = tail.load(std::memory_order_acquire);
        return (currentTail - currentHead + Capacity) & mask;
    }

    size_t capacity() const {
        return Capacity;
    }

private:
    //std::vector<T, std::aligned_storage_t<sizeof(T), alignof(T)>> data;
    std::vector<T> data;
    std::atomic<size_t> head, tail;
    size_t mask;
};

#endif // QUEUE_H
