#include "compute_pool.hpp"

jl::ComputeThreadPool &jl::ComputeThreadPool::GetInstance()
{
    static ComputeThreadPool pool;
    return pool;
}

void jl::ComputeThreadPool::Stop()
{
    bool expect = false;
    if (stop_.compare_exchange_strong(expect, true))
    {
        cond_.notify_all();
        for (int i = 0; i < thread_pool_.size(); ++i)
        {
            if (thread_pool_[i]->joinable())
                thread_pool_[i]->join();
        }
    }
}

jl::ComputeThreadPool::~ComputeThreadPool()
{
    Stop();
}

jl::ComputeThreadPool::ComputeThreadPool(std::size_t thread_cnt)
    : stop_(false)
{
    while (thread_cnt > 0)
    {
        thread_pool_.emplace_back(std::make_unique<std::thread>([=]()
                                                                {
      TaskPtr task;
      while (!stop_) {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            while (task_queue_.empty() && !stop_) {
              cond_.wait(lock);
            }
            if (stop_) {
              break;
            }
            task = task_queue_.front();
            task_queue_.pop();
        }
        (*task)();
      } }));
        --thread_cnt;
    }
}