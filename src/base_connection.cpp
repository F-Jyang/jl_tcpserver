#include "base_connection.h"

namespace jl {

    BaseConnection::BaseConnection(asio::io_context& ioct) :
        shutdown_(false),
        ioct_(ioct),
        io_strand_(ioct),
        timer_(ioct),
        timeout_(kDefaultTimeout)
    {
    }

    void BaseConnection::SetTimeout(std::size_t secs)
    {
        timeout_ = secs;
        auto self = shared_from_this();
        timer_.expires_after(std::chrono::seconds(secs));
        timer_.async_wait(asio::bind_executor(io_strand_, // bind_executor与strand.post的区别 ?
            [self](const std::error_code& ec)
            {
                self->OnTimeout(ec);
            }
        )
        );
    }

    void jl::BaseConnection::CancelTimout()
    {
        timer_.cancel();
    };

    asio::io_context& BaseConnection::GetIoContext()
    {
        return ioct_;
    };


}