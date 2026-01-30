#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>

namespace jl
{
    namespace ssl = asio::ssl;

    using net = asio::ip::tcp;
	using ConstBuffer = asio::const_buffer;
	using MutableBuffer = asio::mutable_buffer;


    class Connection;
      

    using ConnEstablishCallback = std::function<void(net::socket&&)>; // 新连接回调函数
	//using MessageCommingCallback = std::function<void(const std::shared_ptr<Connection> &, ConstBuffer&)>;
	using MessageCommingCallback = std::function<void(const std::shared_ptr<Connection> &, const std::string&)>;
    using WriteFinishCallback = std::function<void(const std::shared_ptr<Connection> &, std::size_t)>;
    using ConnCloseCallback = std::function<void(const std::shared_ptr<Connection> &)>;
    using TimeoutCallback = std::function<void()>;
    using HandshakeCallback = std::function<void(const std::shared_ptr<Connection> &)>;

    // using SslHandshakeCallback = std::function<void(const std::shared_ptr<SslConnection> &)>;
    // using SslMessageCommingCallback = std::function<void(const std::shared_ptr<SslConnection> &, std::size_t, jl::Buffer &)>;
    // using SslWriteFinishCallback = std::function<void(const std::shared_ptr<SslConnection> &, std::size_t)>;
    // using SslConnCloseCallback = std::function<void(const std::shared_ptr<SslConnection> &)>;
    // using SslConnTimeoutCallback = std::function<void(const std::shared_ptr<SslConnection> &)>;

}
