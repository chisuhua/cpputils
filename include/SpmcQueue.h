// spmc_queue.h

#ifndef SPMC_QUEUE_H
#define SPMC_QUEUE_H

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

// Define a default log size for the queue
#ifndef TF_DEFAULT_BOUNDED_TASK_QUEUE_LOG_SIZE
#define TF_DEFAULT_BOUNDED_TASK_QUEUE_LOG_SIZE 10
#endif

// Define a macro for unlikely conditions
#ifndef TF_UNLIKELY
#define TF_UNLIKELY(x) __builtin_expect((x), 0)
#endif

// Define a macro for likely conditions
#ifndef TF_LIKELY
#define TF_LIKELY(x) __builtin_expect((x), 1)
#endif

// Define a macro for cache line size
#ifndef TF_CACHELINE_SIZE
#define TF_CACHELINE_SIZE 64
#endif

/**
@class: SpmcQueue

@tparam T data type
@tparam LogSize the base-2 logarithm of the queue size

@brief class to create a lock-free bounded work-stealing queue

This class implements the work-stealing queue described in the paper, 
"Correct and Efficient Work-Stealing for Weak Memory Models,"
available at https://www.di.ens.fr/~zappa/readings/ppopp13.pdf.

Only the queue owner can perform pop and push operations,
while others can steal data from the queue.
*/
template <typename T, size_t LogSize = TF_DEFAULT_BOUNDED_TASK_QUEUE_LOG_SIZE>
class SpmcQueue {
  
  static_assert(std::is_pointer_v<T>, "T must be a pointer type");
  
  constexpr static int64_t BufferSize_ = int64_t{1} << LogSize;
  constexpr static int64_t BufferMask_ = (BufferSize_ - 1);

  static_assert((BufferSize_ >= 2) && ((BufferSize_ & (BufferSize_ - 1)) == 0));

  alignas(2 * TF_CACHELINE_SIZE) std::atomic<int64_t> head_;
  alignas(2 * TF_CACHELINE_SIZE) std::atomic<int64_t> tail_;
  alignas(2 * TF_CACHELINE_SIZE) std::atomic<T> buffer_[BufferSize_];

  public:
    
  /**
  @brief constructs the queue with a given capacity
  */
  SpmcQueue() = default;

  /**
  @brief destructs the queue
  */
  ~SpmcQueue() = default;
  
  /**
  @brief queries if the queue is empty at the time of this call
  */
  bool empty() const noexcept;
  
  /**
  @brief queries the number of items at the time of this call
  */
  size_t size() const noexcept;

  /**
  @brief queries the capacity of the queue
  */
  constexpr size_t capacity() const;
  
  /**
  @brief tries to insert an item to the queue

  @tparam O data type 
  @param item the item to perfect-forward to the queue
  @return `true` if the insertion succeed or `false` (queue is full)
  
  Only the owner thread can insert an item to the queue. 

  */
  template <typename O>
  bool try_push(O&& item);
  
  /**
  @brief tries to insert an item to the queue or invoke the callable if fails

  @tparam O data type 
  @tparam C callable type
  @param item the item to perfect-forward to the queue
  @param on_full callable to invoke when the queue is full (insertion fails)
  
  Only the owner thread can insert an item to the queue. 

  */
  template <typename O, typename C>
  void push(O&& item, C&& on_full);
  
  /**
  @brief pops out an item from the queue

  Only the owner thread can pop out an item from the queue. 
  The return can be a @c nullptr if this operation failed (empty queue).
  */
  T pop();
  
  /**
  @brief steals an item from the queue

  Any threads can try to steal an item from the queue.
  The return can be a @c nullptr if this operation failed (not necessarily empty).
  */
  T steal();
};

// Function: empty
template <typename T, size_t LogSize>
bool SpmcQueue<T, LogSize>::empty() const noexcept {
  int64_t tail = tail_.load(std::memory_order_relaxed);
  int64_t head = head_.load(std::memory_order_relaxed);
  return tail <= head;
}

// Function: size
template <typename T, size_t LogSize>
size_t SpmcQueue<T, LogSize>::size() const noexcept {
  int64_t tail = tail_.load(std::memory_order_relaxed);
  int64_t head = head_.load(std::memory_order_relaxed);
  return static_cast<size_t>(tail >= head ? tail - head : 0);
}

// Function: try_push
template <typename T, size_t LogSize>
template <typename O>
bool SpmcQueue<T, LogSize>::try_push(O&& o) {

  int64_t tail = tail_.load(std::memory_order_relaxed);
  int64_t head = head_.load(std::memory_order_acquire);

  // queue is full
  if TF_UNLIKELY((tail - head) >= BufferSize_ - 1) {
    return false;
  }
  
  buffer_[tail & BufferMask_].store(std::forward<O>(o), std::memory_order_relaxed);

  std::atomic_thread_fence(std::memory_order_release);
  
  // original paper uses relaxed here but tsa complains
  tail_.store(tail + 1, std::memory_order_release);

  return true;
}

// Function: push
template <typename T, size_t LogSize>
template <typename O, typename C>
void SpmcQueue<T, LogSize>::push(O&& o, C&& on_full) {

  int64_t tail = tail_.load(std::memory_order_relaxed);
  int64_t head = head_.load(std::memory_order_acquire);

  // queue is full
  if TF_UNLIKELY((tail - head) >= BufferSize_ - 1) {
    on_full();
    return;
  }
  
  buffer_[tail & BufferMask_].store(std::forward<O>(o), std::memory_order_relaxed);

  std::atomic_thread_fence(std::memory_order_release);
  
  // original paper uses relaxed here but tsa complains
  tail_.store(tail + 1, std::memory_order_release);
}

// Function: pop
template <typename T, size_t LogSize>
T SpmcQueue<T, LogSize>::pop() {

  int64_t tail = tail_.load(std::memory_order_relaxed) - 1;
  tail_.store(tail, std::memory_order_relaxed);
  std::atomic_thread_fence(std::memory_order_seq_cst);
  int64_t head = head_.load(std::memory_order_relaxed);

  T item {nullptr};

  if(head <= tail) {
    item = buffer_[tail & BufferMask_].load(std::memory_order_relaxed);
    if(head == tail) {
      // the last item just got stolen
      if(!head_.compare_exchange_strong(head, head + 1, 
                                        std::memory_order_seq_cst, 
                                        std::memory_order_relaxed)) {
        item = nullptr;
      }
      tail_.store(tail + 1, std::memory_order_relaxed);
    }
  }
  else {
    tail_.store(tail + 1, std::memory_order_relaxed);
  }

  return item;
}

// Function: steal
template <typename T, size_t LogSize>
T SpmcQueue<T, LogSize>::steal() {
  int64_t head = head_.load(std::memory_order_acquire);
  std::atomic_thread_fence(std::memory_order_seq_cst);
  int64_t tail = tail_.load(std::memory_order_acquire);
  
  T item{nullptr};

  if(head < tail) {
    item = buffer_[head & BufferMask_].load(std::memory_order_relaxed);
    if(!head_.compare_exchange_strong(head, head + 1,
                                      std::memory_order_seq_cst,
                                      std::memory_order_relaxed)) {
      return nullptr;
    }
  }

  return item;
}

// Function: capacity
template <typename T, size_t LogSize>
constexpr size_t SpmcQueue<T, LogSize>::capacity() const {
  return static_cast<size_t>(BufferSize_ - 1);
}

#endif // SPMC_QUEUE_H

