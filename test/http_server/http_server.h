#pragma once

#include <logger.h>
#include <server.h>
#include <http_session.h>
#include <map>
#include <string>
#include <mutex>
#include <util.h>

class HttpServer : public std::enable_shared_from_this<HttpServer>
{
public:
    HttpServer(const std::string& ip, unsigned short port);

    void Start();

    void AppendSession(std::int64_t session_id, const std::shared_ptr<HttpSession>& session);

    void RemoveSession(std::int64_t session_id);

    void Stop();

private:
    std::unique_ptr<jl::util::IdGenerator> id_generator_;
    std::mutex session_mutex_;
    std::map<std::int64_t, std::shared_ptr<HttpSession>> sessions_;
    asio::io_context ioct_;
    jl::Server tcp_server_;
};
