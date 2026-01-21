#include "acceptor.h"
#include <logger.h>
#include <string>

jl::Acceptor::Acceptor(asio::io_context &ioct, const std::string &ip, unsigned short port) : ioct_(ioct),
                                                                                             acceptor_(std::make_unique<net::acceptor>(ioct, net::endpoint(asio::ip::make_address(ip), port)))
{
}

void jl::Acceptor::OnAccept(const std::error_code &ec, net::socket socket)
{
    if (ec)
    { // 如果错误，直接返回
        assert(false);
        LOG_ERROR("OnAccept fail:{}", ec.message());
        return;
    }
    if (conn_establish_callback_)
    {
        conn_establish_callback_(std::move(socket));
    }
    else
    {
        std::error_code ignore_ec;
        socket.shutdown(asio::socket_base::shutdown_both, ignore_ec);
        socket.close(ignore_ec);
    }
    DoAccept(); // 继续接受下一个连接
}

void jl::Acceptor::DoAccept()
{
    if (!acceptor_->is_open())
    {
        assert(false);
        return; // 直接返回
    }

    auto self(shared_from_this()); // 获取自身的shared_ptr，防止在异步操作中被销毁m
    acceptor_->async_accept(std::bind(&Acceptor::OnAccept, self, std::placeholders::_1, std::placeholders::_2));
}

jl::Acceptor::~Acceptor()
{
    acceptor_->close();
}
