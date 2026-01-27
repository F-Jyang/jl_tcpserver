#include "http_server.h"

HttpServer::HttpServer(const std::string& ip, unsigned short port) : tcp_server_(ioct_, ip, port)
{
	tcp_server_.SetConnEstablishCallback([=](jl::Socket&& socket)
		{
			std::shared_ptr<jl::Connection> conn = std::make_shared<jl::Connection>(ioct_, std::move(socket));
			std::shared_ptr<HttpSession> session = std::make_shared<HttpSession>(shared_from_this(), conn);
			this->AppendSession(session);
			session->Start();
			LOG_INFO("New connection comming.");
		}
	);
}

void HttpServer::Start()
{
    tcp_server_.Start();
}

void HttpServer::AppendSession(const std::shared_ptr<HttpSession> &session)
{
	std::lock_guard<std::mutex> lock(session_mutex_);
    sessions_.emplace(session.get(), session);
}

void HttpServer::RemoveSession(const std::shared_ptr<HttpSession> &session)
{
	std::lock_guard<std::mutex> lock(session_mutex_);
	if (sessions_.find(session.get()) != sessions_.end()) {
		sessions_.erase(session.get());
	}
}

void HttpServer::Stop()
{
    tcp_server_.Stop();
}
