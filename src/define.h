#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>

namespace jl
{
    namespace ssl = asio::ssl;

    using net = asio::ip::tcp;
	using Socket = net::socket;
	using SSLSocket = ssl::stream<Socket>;
	using ConstBuffer = asio::const_buffer;
	using MutableBuffer = asio::mutable_buffer;


    class BaseConnection;
    class SslConnection;
    class Buffer;
    class Server;
      

    using ConnEstablishCallback = std::function<void(Socket&&)>; // 新连接回调函数
    using MessageCommingCallback = std::function<void(const std::shared_ptr<BaseConnection> &, ConstBuffer&)>;
    using WriteFinishCallback = std::function<void(const std::shared_ptr<BaseConnection> &, std::size_t)>;
    using ConnCloseCallback = std::function<void(const std::shared_ptr<BaseConnection> &)>;
    using ConnTimeoutCallback = std::function<void(const std::shared_ptr<BaseConnection> &)>;
    using HandshakeCallback = std::function<void(const std::shared_ptr<BaseConnection> &)>;

    // using SslHandshakeCallback = std::function<void(const std::shared_ptr<SslConnection> &)>;
    // using SslMessageCommingCallback = std::function<void(const std::shared_ptr<SslConnection> &, std::size_t, jl::Buffer &)>;
    // using SslWriteFinishCallback = std::function<void(const std::shared_ptr<SslConnection> &, std::size_t)>;
    // using SslConnCloseCallback = std::function<void(const std::shared_ptr<SslConnection> &)>;
    // using SslConnTimeoutCallback = std::function<void(const std::shared_ptr<SslConnection> &)>;

}
