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

template <typename T, size_t N>
class bounded_queue
{
    public:
        using value_type = typename std::array<T,N>::value_type;
        using size_type = typename std::array<T,N>::size_type;

    public:
        bounded_queue () :
            tail_ {0}, head_ {0} {}

    public:
        void push (value_type value)
        {
            size_type head = head_.fetch_add (1, std::memory_order_seq_cst);

            buf_[head % N].first = value;
            buf_[head % N].second = head;
            std::atomic_thread_fence (std::memory_order_release);
        }

        value_type pop ()
        {
            value_type value;
            size_type head, tail, index;
            ssize_t max = N, size = 0; 

            bool head_overrun_condition;
            bool tail_overrun_condition;
            bool index_mistmatch_condition;
            bool retry = false;

            do
            {
                ++poptries;
                tail = tail_.fetch_add (1, std::memory_order_seq_cst);

                do // wait for head to catch up if necessary
                {
                    std::atomic_thread_fence (std::memory_order_acquire);
                    value = buf_[tail % N].first;
                    index = buf_[tail % N].second;

                    head = head_.load (std::memory_order_seq_cst);
                    size = head - tail;

                    head_overrun_condition      = size <= 0;
                    tail_overrun_condition      = size > max;
                    index_mistmatch_condition   = index != tail;

                    if (head_overrun_condition) hoverruns++;
                    if (index_mistmatch_condition) imistmatch++;
                }
                while ((head_overrun_condition || index_mistmatch_condition) && !tail_overrun_condition); 

                if (tail_overrun_condition) 
                {
                    toverruns++;
                    retry = true; 

                    size_type fast_forward_steps = 1;
                    bool tail_bounds_condition;
                    bool tail_unique_condition;

                    do // fast-forward tail to an in bounds index
                    {
                        ++fftries;
                        head = head_.load (std::memory_order_seq_cst);

                        tail_bounds_condition = (head - tail) > max;
                        tail_unique_condition = tail_.compare_exchange_strong (tail, 
                                head - max + fast_forward_steps,
                                std::memory_order_seq_cst); // tail updated in exchange
                        
                        fast_forward_steps = std::min (fast_forward_steps * 2, N / 2);
                    }
                    while (tail_bounds_condition && !tail_unique_condition);
                }
            }
            while (retry);

            return value;
        }

    public:
        size_type size () const { head_ - tail_; }
        bool empty () const { return size() <= 0; }
        bool full () const { return size() >= N; }

        std::array<std::pair<T, size_type>, N> &data () { return buf_; }

    private:
        std::array<std::pair<T, size_type>, N> buf_;

        // eliminate false sharing by giving each variable its own cache line
        union { std::atomic<size_type> tail_;   cacheline padding2; };
        union { std::atomic<size_type> head_;   cacheline padding3; };
        
    public:
        int fftries = 0;
        int poptries = 0;
        int hoverruns = 0;
        int toverruns = 0;
        int imistmatch = 0;
};

int main (int argc, char **argv)
{
    constexpr int itersize = 100000;
    constexpr int datasize = 100;
    constexpr int queuesize = 100000;
    const int nthreads = 3;

    std::atomic<int> start_count {0}, finish_count {0};
    bounded_queue<int,queuesize> queue;

    int output1[datasize];
    int output2[datasize];
    int output3[datasize];

    for (int i=0; i < datasize; ++i)
        output1[i] = output2[i] = 0;

    std::thread producer {[&]
        {
            int i = 0;
            while (start_count < nthreads);
            while (finish_count < nthreads)
            {
                queue.push (i++);
            }
            std::cout << "producer finished" << std::endl;
        }};

    std::thread consumer1 {[&]
        {
            start_count ++;
            for (int i = 0; i < itersize; ++i)
            {
                //std::cout << '.';
                if (queue.empty())
                    output1[i % datasize] = queue.pop ();
            }
            finish_count ++;
            std::cout << "consumer 1 finished" << std::endl;
        }};

    std::thread consumer2 {[&]
        {
            start_count ++;
            for (int i = 0; i < itersize; ++i)
            {
                //std::cout << '!';
                if (queue.empty())
                    output2[i % datasize] = queue.pop ();
            }
            finish_count ++;
            std::cout << "consumer 2 finished" << std::endl;
        }};

    std::thread consumer3 {[&]
        {
            start_count ++;
            for (int i = 0; i < itersize; ++i)
            {
                //std::cout << '?';
                if (queue.empty())
                    output3[i % datasize] = queue.pop ();
            }
            finish_count ++;
            std::cout << "consumer 3 finished" << std::endl;
        }};

    producer.join();
    consumer1.join();
    consumer2.join();
    consumer3.join();

    std::cout << "-----------------------" << std::endl;

    for (int i = 0; i < datasize; ++i)
        std::cout << output1[i] << ", ";
    std::cout << std::endl;

    std::cout << "-----------------------" << std::endl;

    for (int i = 0; i < datasize; ++i)
        std::cout << output2[i] << ", ";
    std::cout << std::endl;

    std::cout << "-----------------------" << std::endl;

    for (int i = 0; i < datasize; ++i)
        std::cout << output3[i] << ", ";
    std::cout << std::endl;

    for (int i = 0; i < datasize; ++i)
        if (std::find (output2, output2 + datasize, output1[i]) != output2 + datasize)
            std::cout << "dupe in output 2: " << output1[i] << std::endl;

    for (int i = 0; i < datasize; ++i)
        if (std::find (output3, output3 + datasize, output1[i]) != output3 + datasize)
            std::cout << "dupe in output 3: " << output1[i] << std::endl;

    for (int i = 0; i < datasize; ++i)
        if (std::find (output1, output1 + datasize, output2[i]) != output1 + datasize)
            std::cout << "dupe in output 1: " << output2[i] << std::endl;

    for (int i = 0; i < datasize; ++i)
        if (std::find (output3, output3 + datasize, output2[i]) != output3 + datasize)
            std::cout << "dupe in output 3: " << output2[i] << std::endl;

    for (int i = 0; i < datasize; ++i)
        if (std::find (output2, output2 + datasize, output3[i]) != output2 + datasize)
            std::cout << "dupe in output 2: " << output3[i] << std::endl;

    for (int i = 0; i < datasize; ++i)
        if (std::find (output1, output1 + datasize, output3[i]) != output1 + datasize)
            std::cout << "dupe in output 1: " << output3[i] << std::endl;

    std::cout << "queue poptries: " << queue.poptries << std::endl;
    std::cout << "queue hoverruns: " << queue.hoverruns << std::endl;
    std::cout << "queue toverruns: " << queue.toverruns << std::endl;
    std::cout << "queue imismatch: " << queue.imistmatch << std::endl;
    std::cout << "queue fftries: " << queue.fftries << std::endl;

    return 0;
}
