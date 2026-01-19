#pragma once

#include <asio.hpp>
#include <asio/ssl.hpp>

namespace jl
{

    class Connection;
    class SslConnection;
    class Buffer;
    class Server;
      
    using net = asio::ip::tcp;
    namespace ssl = asio::ssl;

    using ConnEstablishCallback = std::function<void(const std::shared_ptr<Connection> &)>; // 新连接回调函数
    using MessageCommingCallback = std::function<void(const std::shared_ptr<Connection> &, std::size_t, jl::Buffer &)>;
    using WriteFinishCallback = std::function<void(const std::shared_ptr<Connection> &, std::size_t)>;
    using ConnCloseCallback = std::function<void(const std::shared_ptr<Connection> &)>;
    using ConnTimeoutCallback = std::function<void(const std::shared_ptr<Connection> &)>;

    using SslHandshakeCallback = std::function<void(const std::shared_ptr<SslConnection> &)>;
    using SslMessageCommingCallback = std::function<void(const std::shared_ptr<SslConnection> &, std::size_t, jl::Buffer &)>;
    using SslWriteFinishCallback = std::function<void(const std::shared_ptr<SslConnection> &, std::size_t)>;
    using SslConnCloseCallback = std::function<void(const std::shared_ptr<SslConnection> &)>;
    using SslConnTimeoutCallback = std::function<void(const std::shared_ptr<SslConnection> &)>;

}
