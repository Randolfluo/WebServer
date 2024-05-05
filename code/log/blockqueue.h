
#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H

#include <chrono>
#include <mutex>              //互斥量
#include <deque>              //双端队列
#include <condition_variable> //条件变量
#include <sys/time.h>
#include <assert.h>
//template class<T>          服了，写错了，害我找半天代码补全的问题
template <class T> 
class BlockDeque{
public:
    explicit BlockDeque(size_t MaxCapacity = 1000);

    ~BlockDeque();

    void clear();

    bool empty();

    bool full();

    void Close();       

    ssize_t size();

    ssize_t capacity();

    T front();

    T back();

    void push_back(const T &item);

    void push_front(const T &item);

    bool pop(T &item);

    bool pop(T &item, int timeout);

    void flush();

private:
    std::deque<T> deq_;

    size_t capacity_;

    std::mutex mtx_;

    bool isClose_;

    std::condition_variable condConsumer_;

    std::condition_variable condProducer_;
};

template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity)
:capacity_(MaxCapacity)
{
    assert(MaxCapacity > 0);
    isClose_ = false;
}

template<class T>
BlockDeque<T>::~BlockDeque()
{
    Close();
}

template<class T>
void BlockDeque<T>::clear()
{
    std::lock_guard<std::mutex> locker(mtx_);
    deq_.clear();
}

template<class T>
bool  BlockDeque<T>::empty()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.empty();
}

template<class T>
bool BlockDeque<T>::full()
{
    std::lock_guard<std::mutex> locker(mtx_);
    // if(deq_.size()==capacity_)
    //     return true;
    // return false;
    return deq_.size() >= capacity_;
}

template<class T>
void BlockDeque<T>::Close()
{
    {
        std::lock_guard<std::mutex> locker(mtx_);
        deq_.clear();
        isClose_ = true;
    }
    condProducer_.notify_all();
    condConsumer_.notify_all();
}

template<class T>
ssize_t BlockDeque<T>::size()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.size();
}

template<class T>
ssize_t BlockDeque<T>::capacity()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return capacity_;
}

template<class T>
T BlockDeque<T>::front()
{  
     std::lock_guard<std::mutex> locker(mtx_);
     return deq_.front();
}

template<class T>
T BlockDeque<T>::back()
{
    std::lock_guard<std::mutex> locker(mtx_);
    return deq_.back();
}

template<class T>
void BlockDeque<T>::push_back(const T &item)
{
//考虑容量问题
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_)
    {
        condProducer_.wait(locker);
    }
    deq_.push_back(item);
    condConsumer_.notify_one();
}

template<class T>
void BlockDeque<T>::push_front(const T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.size() >= capacity_)
    {
        condProducer_.wait(locker);
    }
    deq_.push_front(item);
    condConsumer_.notify_one();
}

template<class T>
bool BlockDeque<T>::pop(T &item)
{
    std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty())
    {
        condConsumer_.wait(locker);
        if(isClose_)      return false;
    }
    item = deq_.front();        //注意先取队列再弹出；
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T &item, int timeout)
{
   std::unique_lock<std::mutex> locker(mtx_);
    while(deq_.empty())
    {
        if(condConsumer_.wait_for(locker,std::chrono::seconds(timeout))
            ==  std::cv_status::timeout)           return false;

        if(isClose_)       return false;       
    }
    item = deq_.front();        //注意先取队列再弹出；
    deq_.pop_front();
    condProducer_.notify_one();
    return true;
}

template<class T>
void BlockDeque<T>::flush()
{
  condConsumer_.notify_one();
}

#endif