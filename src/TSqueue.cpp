//Adapted from https://www.geeksforgeeks.org/implement-thread-safe-queue-in-c/

#include <condition_variable>
#include <mutex>
#include "TSqueue.h"

template<typename T>
void TSQueue<T>::push(T item)
{
    // Acquire lock
    std::unique_lock<std::mutex> lock(mMutex);

    // Add item
    mQueue.push(item);

    // Notify one thread that
    // is waiting
    mCond.notify_one();
}

template<typename T>
T TSQueue<T>::pop()
{
    // acquire lock
    std::unique_lock<std::mutex> lock(mMutex);

    // wait until queue is not empty
    mCond.wait(lock,
        [this]() { return !mQueue.empty(); });

    // retrieve item
    T item = mQueue.front();
    mQueue.pop();

    // return item
    return item;
}
