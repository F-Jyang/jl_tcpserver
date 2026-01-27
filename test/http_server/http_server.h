#pragma once

#include <logger.h>
#include <server.h>
#include <http_session.h>
#include <map>
#include <string>
#include <mutex>

class HttpServer : public std::enable_shared_from_this<HttpServer>
{
public:
    HttpServer(const std::string& ip, unsigned short port);

    void Start();

    void AppendSession(const std::shared_ptr<HttpSession>& session);

    void RemoveSession(const std::shared_ptr<HttpSession>& session);

    void Stop();

private:
    std::mutex session_mutex_;
    std::map<HttpSession*, std::shared_ptr<HttpSession>> sessions_;
    asio::io_context ioct_;
    jl::Server tcp_server_;
};
