#pragma once
#include <assert.h>
#include <queue>
#include <stddef.h>
#include <type_traits>

template <typename T> class SlidingAverage {

  public:
    SlidingAverage(size_t N) : window_size(N), average(0) {}

    void Slide(const T &val) {
        static_assert(std::is_floating_point_v<T>,
                      "SlidingAverage only supports floating point types!");

        queue.push(val);
        average += val / window_size;
        if (queue.size() > window_size) {
            const T &front = queue.front();
            average -= front / window_size;
            queue.pop();
        }
    }

    bool Ready() const {
        assert(queue.size() <= window_size);
        return queue.size() == window_size;
    }

    void Resize(size_t new_size) {
        while (new_size < queue.size()) {
            average -= queue.front() / window_size;
            queue.pop();
        }
        average *= window_size;
        average /= new_size;
        window_size = new_size;
    }

    T GetAverage() const { return average; }
    T GetMostRecent() const {return queue.back(); }
    size_t GetSize() const { return window_size; }

  private:
    size_t window_size;
    T average;
    std::queue<T> queue;
};