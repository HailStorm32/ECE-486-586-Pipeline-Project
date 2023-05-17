//Adapted from https://www.geeksforgeeks.org/implement-thread-safe-queue-in-c/

#pragma once

#include <condition_variable>
#include <mutex>
#include <queue>

template <typename T>
class TSQueue {
private:
    // Underlying queue
    std::queue<T> mQueue;

    // mutex for thread synchronization
    std::mutex mMutex;

    // Condition variable for signaling
    std::condition_variable mCond;

public:
    // Pushes an element to the queue
    void push(T item);

    // Pops an element off the queue
    T pop();
};
