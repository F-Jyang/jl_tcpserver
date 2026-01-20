#pragma once
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#define LOG_DEBUG(FORMAT, ...) jl::Logger::GetInstance().LogDebug(fmt::format(FORMAT, __VA_ARGS__))
#define LOG_INFO(FORMAT, ...) jl::Logger::GetInstance().LogInfo(fmt::format(FORMAT, __VA_ARGS__))
#define LOG_WARN(FORMAT, ...) jl::Logger::GetInstance().LogWarn(fmt::format(FORMAT, __VA_ARGS__))
#define LOG_ERROR(FORMAT, ...) jl::Logger::GetInstance().LogError(fmt::format(FORMAT, __VA_ARGS__))

namespace jl {
    /// <summary>
    /// 设置单个文件最大size
    /// </summary>
    /// <param name="size"></param>
    void SetLogFileMaxSize(int size);

    /// <summary>
    /// 设置log文件最大个数
    /// </summary>
    /// <param name="files"></param>
    void SetLogMaxFiles(int files);

    /// <summary>
    /// 设置日志刷新间隔
    /// </summary>
    /// <param name="interval"></param>
    void SetLogFlushInterval(int interval);

    /// <summary>
    /// 设置日志同步或异步
    /// </summary>
    /// <param name="block"></param>
    void SetLogBlock(bool block);

    /// <summary>
    /// 设置日志的线程池
    /// </summary>
    /// <param name="q_size">队列容量</param>
    /// <param name="thread_count">线程数量</param>
    void SetLogThreadPool(int q_size, int thread_count);


    class BaseLoggerImpl
    {
    public:
        BaseLoggerImpl(const std::shared_ptr<spdlog::logger>& logger) : logger_(logger) {}

        virtual void LogDebug(const std::string& msg) = 0;
        virtual void LogInfo(const std::string& msg) = 0;
        virtual void LogWarn(const std::string& msg) = 0;
        virtual void LogError(const std::string& msg) = 0;

        /// <summary>
        /// SPDLOG_LOGGER_XXXX 宏需要调用该函数，也是实际调用的log函数。以上四个函数并没有被调用（无法打印源文件、函数和行号）
        /// </summary>
        /// <param name="source">文件名、函数、行号</param>
        /// <param name="lvl">level</param>
        /// <param name="msg">日志内容</param>
        virtual void log(spdlog::source_loc&& source, spdlog::level::level_enum lvl, const std::string& msg) = 0;

        virtual void Flush() = 0;

    protected:
        std::shared_ptr<spdlog::logger> logger_;
    };

    class Logger
    {
    public:
        static Logger& GetInstance();

        void LogDebug(const std::string& msg)
        {
            // logger_impl_->LogDebug(msg); // 无法打印文件、函数、行号
            SPDLOG_LOGGER_DEBUG(logger_impl_, msg);
        }

        void LogInfo(const std::string& msg)
        {
            SPDLOG_LOGGER_INFO(logger_impl_, msg);
            // logger_impl_->LogInfo(msg);
        }

        void LogWarn(const std::string& msg)
        {
            // logger_impl_->LogWarn(msg);
            SPDLOG_LOGGER_WARN(logger_impl_, msg);
        }

        void LogError(const std::string& msg)
        {
            // logger_impl_->LogError(msg);
            SPDLOG_LOGGER_ERROR(logger_impl_, msg);
        }

        void Flush();

        ~Logger()
        {
            // LOG_DEBUG("~ToolLogger");
            //  spdlog::shutdown();
        }

    private:
        Logger() : logger_impl_(nullptr) {};

    private:
        std::unique_ptr<BaseLoggerImpl> logger_impl_;
    };
}