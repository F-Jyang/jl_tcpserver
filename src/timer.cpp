#include "timer.h"
#include <logger.h>

namespace jl
{
    Timer::Timer(const std::shared_ptr<IConnection>& conn) :
        timer_(conn->GetExecutor())
    {
    }

    Timer::Timer(const asio::any_io_executor& executor) :
        timer_(executor)
    {
    }

    void Timer::Wait(std::size_t milli_secs)
    {
        auto self = shared_from_this();
        timer_.expires_after(std::chrono::milliseconds(milli_secs));
        timer_.async_wait(
            [=](const std::error_code& ec)
            {
                if (!ec) // 正常超时
                {
                    if (self->callback_)
                    {
                        self->callback_();
                    }
                }
                else if (ec && ec != asio::error::operation_aborted)
                {
                    LOG_ERROR("OnTimeout error: {}.", ec.message());
                }
                else
                {
#ifdef _DEBUG
                    LOG_WARN("OnTimeout operation_aborted.");
#endif // _DEBUG
                }
            });
    }
    
    void Timer::Cancel()
    {
        timer_.cancel();
    }

}