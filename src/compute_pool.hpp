/// @file compute_pool.hpp
/// @brief 计算线程池类
/// @author Jyang.
/// @date 2026-1-17
/// @version 1.0

#pragma once

#include <vector>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <future>
#include <thread>
#include <memory>

namespace jl
{
    class ComputeThreadPool
    {
        using Task = std::packaged_task<void()>;
        using TaskPtr = std::shared_ptr<Task>;

    public:
        static ComputeThreadPool &GetInstance();

        ComputeThreadPool(const ComputeThreadPool &) = delete;

        ComputeThreadPool &operator=(const ComputeThreadPool &) = delete;

        /// @brief 提交任务
        /// @param task_ptr 任务指针
        void Post(const TaskPtr &task_ptr)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            task_queue_.emplace(std::move(task_ptr));
            cond_.notify_one();
        }

        /// @brief 提交任务
        /// @tparam Func 函数类型
        /// @tparam Args 参数类型
        /// @param f 函数对象
        /// @param args 参数对象
        template <typename Func, typename... Args>
        void Post(Func &&f, Args &&...args)
        {
            TaskPtr task_ptr = std::make_shared<Task>(std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
            // this->Post(task_ptr); // bug: error
            std::unique_lock<std::mutex> lock(mutex_);
            task_queue_.emplace(std::move(task_ptr));
            cond_.notify_one();
        }

        /// @brief 停止线程池
        void Stop();

        ~ComputeThreadPool();

    private:
        ComputeThreadPool(std::size_t thread_cnt = std::thread::hardware_concurrency());

    private:
        std::atomic<bool> stop_;
        std::mutex mutex_;
        std::condition_variable cond_;
        std::queue<TaskPtr> task_queue_;
        std::vector<std::unique_ptr<std::thread>> thread_pool_;
    };
}