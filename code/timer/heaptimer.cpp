#include "heaptimer.h"


HeapTimer::HeapTimer() 
{ heap_.reserve(64); }

HeapTimer::~HeapTimer() 
{
    clear();
}
//调整指定id的节点
void HeapTimer::adjust(int id, int timeout)
{
   assert(!heap_.empty() && ref_.count(id) > 0);
   heap_[ref_[id]].expires = Clock::now() + MS(timeout);
   siftdown_(ref_[id], heap_.size());
}

void HeapTimer::add(int id, int timeOut, const TimeoutCallBack &cb)
{
    assert(id >= 0);
    if(ref_.count(id) == 0)
    {
        size_t i = heap_.size();
        ref_[id] = i;
        heap_.push_back({id,Clock::now()+ MS(timeOut),cb});
        siftup_(i);
    }
    else
    {
        size_t i = ref_[id];
        heap_[i].expires = Clock::now() + MS(timeOut);
        heap_[i].cb = cb;        //这里与adjust不同，要重新设置回调函数
        if(!siftdown_(i, heap_.size()))
            siftup_(i);
    }
}

void HeapTimer::doWork(int id){
    if(heap_.empty() || ref_.count(id)==0)  return;
    
    size_t i = ref_[id];
    TimerNode node = heap_[i];
    node.cb();
    del_(i);
}

void HeapTimer::clear()
{
    ref_.clear();
    heap_.clear();
}

void HeapTimer::tick()
{
    if(heap_.empty())   return;
    while(!heap_.empty())
    {
        TimerNode node = heap_.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) break;
        node.cb();
        pop();
    }
}

void HeapTimer::pop(){
    assert(!heap_.empty());
    del_(0);
}

//清除超时计时器并返回下一计时器所剩时间
int HeapTimer::GetNextTick()
{
    tick();
    int res = -1;
    //原文类型是size_t(无符号整型),但是若res为负数，则为无穷大，下面的判断就不起作用了
    //注意ssize_t是有符号整型，size_t则是无符号整型
    if(!heap_.empty())
    {
        res =std::chrono::duration_cast<MS>(heap_.front().expires - Clock::now()).count();
       if(res < 0)
    {
        res = 0;
    }
    }
 
    return res;
}

//我们需要确保树是满树，因此我们需要在vector尾部移除这一元素
void HeapTimer::del_(size_t index)
{
    assert(!heap_.empty() && index >= 0 && index< heap_.size());
    size_t i = index;
    size_t n = heap_.size()- 1;
    assert(i<=n);


    if(i < n) {
        SwapNode_(i, n);
          //因为节点n的时间肯定是大于等于i的，所以不会将n处要删除的节点替换掉
        if(!siftdown_(i, n)) {
            siftup_(i);
        }
    }
    /* 队尾元素删除 */
    ref_.erase(heap_.back().id);
    heap_.pop_back();
}
//向上调整
void HeapTimer::siftup_(size_t i)
{
    assert(i >= 0 && i < heap_.size());
    size_t j = (i-1)/2;
    while(j >= 0)
    {
        if(heap_[i] > heap_[j]) break;
        SwapNode_(i, j);
        i = j;
        j = (i-1) /2;
    }

}


//向下调整
bool HeapTimer::siftdown_(size_t index, size_t n)
{
    assert(index >= 0 && index < heap_.size());
    assert(n >= 0 && n <= heap_.size());
    size_t i = index;
    size_t j = 2*i + 1;    //左节点
    
    while(j < n)
    {
        if(j+1 < n && heap_[j+1] < heap_[j]) j++;   //比较左右节点
        if(heap_[i] < heap_[j]) break;
        SwapNode_(i,j);
        i = j;
        j = 2*i + 1;  
    }   
    return i > index; //返回是否交换节点
}

void HeapTimer::SwapNode_(size_t i, size_t j)
{
    assert(i >= 0 && i < heap_.size());
    assert(j >= 0 && j < heap_.size());
    std::swap(heap_[i],heap_[j]);
    //i，j在heap_的位置已经变化
    ref_[heap_[i].id] = i;
    ref_[heap_[j].id] = j;
}

