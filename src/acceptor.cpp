#include "acceptor.h"
#include <string>

jl::Acceptor::Acceptor(asio::io_context &ioct, const std::string &ip, unsigned short port) : ioct_(ioct),
                                                                                             acceptor_(std::make_unique<net::acceptor>(ioct, net::endpoint(asio::ip::make_address(ip), port)))
{
}

void jl::Acceptor::OnAccept(const std::error_code &ec, net::socket socket)
{
    if (ec)
    { // 如果错误，直接返回
        return;
    }
    std::shared_ptr<Connection> conn = std::make_shared<Connection>(ioct_, std::move(socket));
    if (conn_establish_callback_)
    {
        conn_establish_callback_(conn);
    }
    else
    {
        conn->Close();
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
