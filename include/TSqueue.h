//Adapted from https://www.geeksforgeeks.org/implement-thread-safe-queue-in-c/

#pragma once

#include <condition_variable>
#include <queue>
#include <mutex>

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

template<typename T>
inline void TSQueue<T>::push(T item)
{
    // Acquire lock
    std::unique_lock<std::mutex> lock(mMutex);

    // Add item
    mQueue.push(item);

    //// Notify one thread that
    //// is waiting
    //mCond.notify_one();
}

template<typename T>
inline T TSQueue<T>::pop()
{
    // acquire lock
    std::unique_lock<std::mutex> lock(mMutex);

    //// wait until queue is not empty
    //mCond.wait(lock,
    //    [this]() { return !mQueue.empty(); });

    if (mQueue.empty())
    {
        return NULL;
    }

    // retrieve item
    T item = mQueue.front();
    mQueue.pop();

    // return item
    return item;
}
