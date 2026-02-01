#pragma once

#include <http_utils.h>
#include <timer.h>
#include <util.h>

#include <connection.h>

class HttpServer;

class HttpSession :public std::enable_shared_from_this<HttpSession> {
    enum class State {
        kRequestLine,
        kHeaders,
        kBody,
        kDone,
    };

public:
    HttpSession(const std::shared_ptr<HttpServer>& server, std::int64_t id, const std::shared_ptr<jl::IConnection>& conn) :
        state_(State::kRequestLine),
        server_(server),
        session_id_(id),
        conn_(conn),
        timer_(std::make_shared<jl::Timer>(conn)),
        remote_ip_(conn->GetRemoteEndpoint().address().to_string()),
        remote_port_(conn->GetRemoteEndpoint().port())
    {
    }

    void Start();
    
    ~HttpSession();

private:
    void OnTimeout();

private:
    std::int64_t session_id_;
    std::shared_ptr<jl::Timer> timer_;
    const std::string remote_ip_;
    const unsigned short remote_port_;
    State state_;
    std::shared_ptr<jl::IConnection> conn_;
    HttpRequest request_;
    //HttpResponse response_;
    std::weak_ptr<HttpServer> server_;
};
    