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
        using Task = std::function<void()>;
    public:
        static ComputeThreadPool &GetInstance();

        ComputeThreadPool(const ComputeThreadPool&) = delete;
        ComputeThreadPool(ComputeThreadPool &&) = delete;

        ComputeThreadPool& operator=(const ComputeThreadPool&) = delete;
        ComputeThreadPool &operator=(ComputeThreadPool &&) = delete;

#if (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || \
    (defined(__cplusplus) && __cplusplus >= 201703L) // 启用了 C++17 或以上标准
		/// @brief 提交任务
		/// @tparam Func 函数类型
		/// @tparam Args 参数类型
		/// @param f 函数对象
		/// @param args 参数对象
		template <typename Func, typename... Args>
        auto Post(Func&& f, Args &&...args)->std::future<std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>>
		{
            // 使用 std::tuple + std::index_sequence 对比其他处理args...的方式，有点在于：可以对参数进行检验、更易于调试。cpp17之后更推荐这种方式
            //auto tuples = std::forward_as_tuple(args...);  // 将 args 转换为std::tuple
			//std::cout << typeid(std::get<1>(tuples)).name() << std::endl;
			//std::cout << typeid(std::get<2>(tuples)).name() << std::endl;
            using ReturnType = std::invoke_result_t<std::decay_t<Func>, std::decay_t<Args>...>;
            auto task_ptr = std::make_shared<std::packaged_task<ReturnType(void)>>(
                [func = std::forward<Func>(f), argsTuple = std::make_tuple(std::forward<Args>(args)...)]() mutable
                {
                    return std::apply(std::move(func), std::move(argsTuple));
                }
            );
			{
				std::unique_lock<std::mutex> lock(mutex_);
				task_queue_.emplace([task_ptr]()
					{
						(*task_ptr)();
					});
			}
			cond_.notify_one();
			return task_ptr->get_future();
		}

#else
		/// @brief 提交任务
		/// @tparam Func 函数类型
		/// @tparam Args 参数类型
		/// @param f 函数对象
		/// @param args 参数对象
		template <typename Func, typename... Args>
		auto Post(Func&& f, Args &&...args) -> std::future<std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>>
		{
			using ReturnType = std::result_of_t<std::decay_t<Func>(std::decay_t<Args>...)>;
			auto task_ptr = std::make_shared<std::packaged_task<ReturnType(void)>>(std::bind(std::forward<Func>(f), std::forward<Args>(args)...));
			{
				std::unique_lock<std::mutex> lock(mutex_);
				task_queue_.emplace([task_ptr]()
					{
						(*task_ptr)();
					});
			}
			cond_.notify_one();
			return task_ptr->get_future();
		}
#endif

        //// 提交任务，返回 future
        //template <typename Func, typename... Args>
        //auto Post(Func&& f, Args&&... args)
        //    -> std::future<typename std::result_of<
        //    typename std::decay<Func>::type(typename std::decay<Args>::type...)
        //    >::type>
        //{
        //    using ReturnType = typename std::result_of<
        //        typename std::decay<Func>::type(typename std::decay<Args>::type...)
        //    >::type;

        //    // 将函数和参数打包到 tuple（值语义，避免引用悬挂）
        //    using TaskType = std::packaged_task<ReturnType()>;

        //    // 使用 lambda 捕获 tuple，延迟执行时展开
        //    auto bound_args = std::make_shared<std::tuple<
        //        typename std::decay<Func>::type,
        //        typename std::decay<Args>::type...
        //        >>(
        //            std::forward<Func>(f),
        //            std::forward<Args>(args)...
        //            );

        //    auto task = std::make_shared<TaskType>([bound_args]() mutable {
        //        return ApplyImpl<ReturnType>(bound_args.get(),
        //            std::index_sequence_for<Args...>{});
        //        });

        //    std::future<ReturnType> result = task->get_future();

        //    {
        //        std::unique_lock<std::mutex> lock(mutex_);
        //        if (stop_) {
        //            throw std::runtime_error("ThreadPool stopped");
        //        }
        //        task_queue_.emplace([task]() { (*task)(); });
        //    }

        //    cond_.notify_one();
        //    return result;
        //}


        //// 展开 tuple 调用函数：第0个元素是函数，其余是参数
        //template <typename ReturnType, typename Tuple, std::size_t... Indices>
        //static ReturnType ApplyImpl(Tuple* t, std::index_sequence<Indices...>) {
        //    return std::get<0>(*t)(std::get<Indices + 1>(std::move(*t))...);
        //}

        /// @brief 停止线程池
        void Stop();

        ~ComputeThreadPool();

    private:
        ComputeThreadPool(std::size_t thread_cnt = std::thread::hardware_concurrency());

    private:
        std::atomic<bool> stop_;
        std::mutex mutex_;
        std::condition_variable cond_;
        std::queue<Task> task_queue_;
        std::vector<std::unique_ptr<std::thread>> thread_pool_;
    };
}