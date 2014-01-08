#include <iostream>
#include <algorithm>
#include <cassert>
#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

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

template <typename T, size_t N>
class bounded_queue
{
    public:
        using value_type = typename std::array<T,N>::value_type;
        using size_type = typename std::array<T,N>::size_type;

    public:
        bounded_queue () :
            head_ {0}, tail_ {0}, size_ {0} {}

    public:
        void push (value_type const &value)
        {
            auto head = head_++;
            while (size_ >= N && (head - tail_) >= N);
            buffer_[head % N] = value;
            size_++;
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
            auto tail = tail_++;
            while (size_ <= 0 && (head_ - tail) <= 0);
            value = buffer_[tail % N];
            size_--;
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
        size_type size () const { return size_; }
        size_type full () const { return size_ == N; }
        size_type empty () const { return size_ == 0; }

    private:
        // eliminate false sharing by giving each variable its own cache line
        union { std::atomic<uint64_t> tail_;    cacheline padding1; };
        union { std::atomic<uint64_t> head_;    cacheline padding2; };
        union { std::atomic<int32_t> size_;     cacheline padding3; };

    private:
        std::array<T,N> buffer_;

};

int main (int argc, char **argv)
{
    bounded_queue<int, 10> queue;

    std::atomic<bool> start {false};

    size_t const consumption = 100000;
    size_t const production = 100000;

    std::thread producer {[&]
        {
            start = true;
            std::cout << "starting producer" << std::endl;

            for (int i=0; i < production; ++i)
                queue.push (i);

            std::cout << "finished producer" << std::endl;
        }};

    std::thread consumer {[&]
        {
            int result = 0;
            while (start == false);
            std::cout << "starting consumer" << std::endl;

            for (int i=0; i < consumption; ++i)
            {
                queue.pop (result);
                if (result != i) 
                    std::cout << result << " / " << i << std::endl;
            }

            std::cout << std::endl;
            std::cout << "finished consumer" << std::endl;
        }};

    producer.join();
    consumer.join();

    return 0;
}
