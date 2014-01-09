#include <iostream>
#include <cassert>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>
#include <algorithm>

constexpr static size_t cache_width = 64;

struct alignas (cache_width) cacheline
{
    uint8_t padding [cache_width];
};

class spin_mutex
{
    public:
        void lock () { while (locked_.exchange (true)); }
        void unlock () { locked_ = false; }

    private:
        std::atomic<bool> locked_ {false};
};

// Multi-producer/Multi-consumer fixed size cirucular queue
// push-on-full and pop-on-empty will spin; try_push/pop will return wait free
// empty and full are disambiguated using a size count
// sequentially consistent atomics are used

template <typename T, int N>
class bounded_queue
{
    public:
        using value_type = typename std::array<T,N>::value_type;
        using size_type = typename std::array<T,N>::size_type;

    public:
        bounded_queue () :
#ifdef TEST
            head_ {0}, tail_ {0}, size_ {0} {}
#else
            head_ {0}, tail_ {0}, head_ticket_ {0}, tail_ticket_ {0} {}
#endif

    public:
        void push (value_type const &value)
        {
#ifdef TEST
            std::unique_lock<std::mutex> lock {mutex};
            available_room.wait (lock, [this] { return !full(); });

            buffer_[head_++ % N] = value;
            size_++;

            available_data.notify_one();
#else
            auto ticket = head_ticket_.fetch_add (1);
            auto head = head_.load(); 
            auto tail = tail_.load();

            while ((head - tail) >= N || ticket != head)
                head = head_.load(), tail = tail_.load();

            std::atomic_thread_fence (std::memory_order_acquire);

            assert (size() >= 0 && size() <= N);
            buffer_[ticket % N] = value;

            std::atomic_thread_fence (std::memory_order_release);

            head_++;
#endif
        }

        bool try_push (value_type const &value)
        {
            if (full ()) return false;
            else
            {
                push (value); 
                return true;
            }
        }

        void pop (value_type &value)
        {
#ifdef TEST
            std::unique_lock<std::mutex> lock {mutex};
            available_data.wait (lock, [this] { return !empty(); });

            value = buffer_[tail_++ % N];
            size_--;

            available_room.notify_one();
#else
            auto ticket = tail_ticket_.fetch_add (1);
            auto head = head_.load(); 
            auto tail = tail_.load();

            while ((head - tail) <= 0 || ticket != tail)
                head = head_.load(), tail = tail_.load();

            std::atomic_thread_fence (std::memory_order_acquire);

            assert (size() >= 0 && size() <= N);
            value = buffer_[tail % N];
            
            std::atomic_thread_fence (std::memory_order_release);

            tail_++;
#endif
        }

        bool try_pop (value_type &value)
        {
            if (empty ()) return false;
            else
            {
                pop (value); 
                return true;
            }
        }

    public:
        size_type size () const { return head_ - tail_; }
        size_type full () const { return size() == N; }
        size_type empty () const { return size() == 0; }

    private:
#ifdef TEST
        int64_t tail_;
        int64_t head_;
        int32_t size_;
        std::mutex mutex;
        std::condition_variable available_data;
        std::condition_variable available_room;
#else
        std::atomic<int64_t> head_;
        std::atomic<int64_t> tail_;

        std::atomic<int64_t> head_ticket_;
        std::atomic<int64_t> tail_ticket_;
#endif

    private:
        std::array<T,N> buffer_;

};

int main (int argc, char **argv)
{
    bounded_queue<int, 10> queue;
    std::atomic<int> product {0};
    std::atomic<int> consumed {0};
    size_t const production = 100000000;

    //std::vector<std::atomic<bool>> found (production);
    //for (auto &v : found) v = false;

    std::thread producer1 {[&]
        {
            std::cout << "starting producer" << std::endl;

            int value = 0;
            while ((value = product++) < production)
                queue.push (value);

            std::cout << "finished producer" << std::endl;
        }};

    std::thread producer2 {[&]
        {
            std::cout << "starting producer" << std::endl;

            int value = 0;
            while ((value = product++) < production)
                queue.push (value);

            std::cout << "finished producer" << std::endl;
        }};

    std::thread producer3 {[&]
        {
            std::cout << "starting producer" << std::endl;

            int value = 0;
            while ((value = product++) < production)
                queue.push (value);

            std::cout << "finished producer" << std::endl;
        }};

    std::thread consumer1 {[&]
        {
            int result = 0;
            while (consumed++ < production)
            {
                queue.pop (result);
                //found[result] = true;
            }

            std::cout << result << std::endl;
            std::cout << "finished consumer" << std::endl;
        }};

    std::thread consumer2 {[&]
        {
            int result = 0;
            while (consumed++ < production)
            {
                queue.pop (result);
                //found[result] = true;
            }

            std::cout << result << std::endl;
            std::cout << "finished consumer" << std::endl;
        }};

    producer1.join();
    producer2.join();
    producer3.join();
    consumer1.join();
    consumer2.join();

    //int i = 0;
    //for (auto &v : found)
    //    std::cout << i++ << " = " << v << std::endl;

    return 0;
}
