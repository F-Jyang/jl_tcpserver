#include "logger.h"

#include <global.h>

namespace jl {

    int kLogMaxSize = 1024 * 1024 * 20;
    int kLogMaxFiles = 3;
    int kLogFlushInterval = 3;
    bool kLogIsBlock = false;
    int kLogThreadCount = 1;
    int kLogQueueSize = 2048;
    // std::string kDefaultLogFile = fmt::format("{}/{}", GetDataRoot(), "logs/easy_tools_log.txt");
    std::string kDefaultLogFile = fmt::format("{}/{}", "./logs", "jl_tcpserver.txt");
    constexpr const char* kAsyncLoggerName = "async_jl_tcpserver_logger";
    constexpr const char* kSyncLoggerName = "sync_jl_tcpserver_logger";
    constexpr const char* LOG_FORMAT = "%^[%Y-%m-%d %H:%M:%S.%e][thread %t][%l]: %v%$"; // %^...%$ 打印颜色

    void SetLogFileMaxSize(int size) {
        kLogMaxSize = size;
    }

    void SetLogMaxFiles(int files) {
        kLogMaxFiles = files;
    }

    void SetLogFlushInterval(int interval) {
        kLogFlushInterval = interval;
    }

    void SetLogBlock(bool block)
    {
        kLogIsBlock = block;
    }

    void SetLogThreadPool(int q_size, int thread_count)
    {
        kLogQueueSize = q_size;
        kLogThreadCount = thread_count;
    }

    class AsyncLoggerImpl : public BaseLoggerImpl
    {
    public:
        AsyncLoggerImpl(const std::string& logger_name, spdlog::sinks_init_list& sink_list) :
            BaseLoggerImpl(nullptr)
        {
            logger_ = std::make_shared<spdlog::async_logger>(
                logger_name, sink_list.begin(), sink_list.end(),
                spdlog::thread_pool(), spdlog::async_overflow_policy::block);
#ifdef _DEBUG
            logger_->set_level(spdlog::level::debug);
#else
            logger_->set_level(spdlog::level::info);
#endif
            spdlog::register_logger(logger_);
        }

        void LogDebug(const std::string& msg) override
        {
            logger_->debug(msg);
        };

        void LogInfo(const std::string& msg) override
        {
            logger_->info(msg);
        };

        void LogWarn(const std::string& msg) override
        {
            logger_->warn(msg);
        };

        void LogError(const std::string& msg) override
        {
            logger_->error(msg);
        };

        void log(spdlog::source_loc&& source, spdlog::level::level_enum lvl, const std::string& msg) override
        {
            logger_->log(std::move(source), lvl, msg);
        }

        void Flush()
        {
            logger_->flush();
        }

        ~AsyncLoggerImpl()
        {
            // spdlog::drop_all();
            if (logger_)
            {
                logger_->flush();
                spdlog::drop(logger_->name());
            }
            // spdlog::shutdown();
        }
    };

    class SyncLoggerImpl : public BaseLoggerImpl {
    public:
        SyncLoggerImpl(const std::string& logger_name, std::initializer_list<spdlog::sink_ptr>& sink_list) :
            BaseLoggerImpl(nullptr)
        {
            logger_ = std::make_shared<spdlog::logger>(logger_name, sink_list);
#ifdef _DEBUG
            logger_->set_level(spdlog::level::debug);
#else
            logger_->set_level(spdlog::level::info);
#endif
            spdlog::register_logger(logger_);
        }

        void LogDebug(const std::string& msg) override
        {
            logger_->debug(msg);
        };

        void LogInfo(const std::string& msg) override
        {
            logger_->info(msg);
        };

        void LogWarn(const std::string& msg) override
        {
            logger_->warn(msg);
        };

        void LogError(const std::string& msg) override
        {
            logger_->error(msg);
        };

        void log(spdlog::source_loc&& source, spdlog::level::level_enum lvl, const std::string& msg) override
        {
            logger_->log(std::move(source), lvl, msg);
        }

        void Flush()
        {
            logger_->flush();
        }

        ~SyncLoggerImpl()
        {
            if (logger_) {
                spdlog::drop(logger_->name());
            }
            // spdlog::shutdown();
        }
    };

    Logger& Logger::GetInstance()
    {
        static std::once_flag flag;
        static Logger instance;
        std::call_once(flag, [&]()
            {
                spdlog::init_thread_pool(kLogQueueSize, kLogThreadCount);
                spdlog::flush_every(std::chrono::seconds(kLogFlushInterval));
                // 创建文件输出sink
                auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(kDefaultLogFile, kLogMaxSize, kLogMaxFiles);
                file_sink->set_pattern(LOG_FORMAT);
                // 创建控制台输出sink
                auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>(spdlog::color_mode::automatic);
                console_sink->set_pattern(LOG_FORMAT);
                // 创建异步多sink 异步logger
                spdlog::sinks_init_list sinks = { console_sink, file_sink };
                if (kLogIsBlock) {
                    instance.logger_impl_ = std::make_unique<SyncLoggerImpl>(kAsyncLoggerName, sinks);
                }
                else { // 默认走unblock
                    instance.logger_impl_ = std::make_unique<AsyncLoggerImpl>(kSyncLoggerName, sinks);
                }
#ifdef _DEBUG
                for (auto& sink : sinks) {
                    sink->set_level(spdlog::level::debug);
                }
#else
                for (auto& sink : sinks) {
                    sink->set_level(spdlog::level::info);
                }
#endif
            });
        return instance;
    }


    void Logger::Flush()
    {
        if (logger_impl_)
        {
            logger_impl_->Flush();
        }
    }
}