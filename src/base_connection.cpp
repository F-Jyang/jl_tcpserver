#include "base_connection.h"

namespace jl {

    BaseConnection::BaseConnection(asio::io_context& ioct, std::size_t buffer_max_size) :
        ioct_(ioct),
        state_(ConnectionState::kActived),
        io_strand_(asio::make_strand(ioct)),
        timer_(ioct),
        timeout_(kDefaultTimeout),
        read_buffer_(buffer_max_size),
        read_in_progress_(false)
    {
    }

    void BaseConnection::SetTimeout(std::size_t secs)
    {
        timeout_ = secs;
    }

    void jl::BaseConnection::CancelTimer()
    {
        timer_.cancel();
    }

    void BaseConnection::StartTimer()
    {
        auto self = shared_from_this();
        timer_.expires_after(std::chrono::seconds(timeout_));
        timer_.async_wait(asio::bind_executor(io_strand_, // bind_executor与strand.post的区别 ?
            [self](const std::error_code& ec)
            {
                self->OnTimeout(ec);
            })
        );
    }

    asio::io_context& BaseConnection::GetIoContext()
    {
        return ioct_;
    };


}