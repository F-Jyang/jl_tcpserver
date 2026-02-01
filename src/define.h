#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>

namespace jl
{
    namespace ssl = asio::ssl;

    using net = asio::ip::tcp;
	using ConstBuffer = asio::const_buffer;
	using MutableBuffer = asio::mutable_buffer;


    class IConnection;
      

    using ConnEstablishCallback = std::function<void(net::socket&&)>; // 新连接回调函数

    using MessageCommingCallback = std::function<void(const std::shared_ptr<IConnection> &, const std::string&)>;
    using WriteFinishCallback = std::function<void(const std::shared_ptr<IConnection> &, std::size_t)>;
    using ConnCloseCallback = std::function<void(const std::shared_ptr<IConnection> &)>;
    using HandshakeCallback = std::function<void(const std::shared_ptr<IConnection> &)>;

    using TimeoutCallback = std::function<void()>;

}
