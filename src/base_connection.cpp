#include "base_connection.h"

namespace jl {

    BaseConnection::BaseConnection(asio::io_context& ioct, std::size_t buffer_max_size) :
        ioct_(ioct),
        state_(ConnectionState::kActived),
        read_buffer_(buffer_max_size),
        read_in_progress_(false)
        //io_strand_(asio::make_strand(ioct)),
        //timer_(ioct),
        //set_timeout_(false),
    {
    }

    //void jl::BaseConnection::CancelTimer()
    //{
    //    set_timeout_ = false;
    //    timer_.cancel();
    //}

    //void BaseConnection::StartTimer(std::size_t milli_secs)
    //{
    //    set_timeout_ = true;
    //    auto self = shared_from_this();
    //    timer_.expires_after(std::chrono::milliseconds(milli_secs));
    //    timer_.async_wait(asio::bind_executor(io_strand_, // bind_executor与strand.post的区别 ?
    //        [self](const std::error_code& ec)
    //        {
    //            self->OnTimeout(ec);
    //        })
    //    );
    //}

    asio::io_context& BaseConnection::GetIoContext()
    {
        return ioct_;
    };


}