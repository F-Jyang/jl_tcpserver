#include "http_server.h"

HttpServer::HttpServer(const std::string& ip, unsigned short port) :
	tcp_server_(ioct_, ip, port),
	id_generator_(jl::util::MakeIdGenerator<jl::util::Snowflake>(1))
{
	tcp_server_.SetConnEstablishCallback([=](jl::net::socket&& socket)
		{
			std::shared_ptr<jl::IConnection> conn = jl::MakeSSLConnection(std::move(socket));
			std::int64_t session_id = id_generator_->GenerateId();
			std::shared_ptr<HttpSession> session = std::make_shared<HttpSession>(shared_from_this(), session_id, conn);
			this->AppendSession(session_id, session);
			session->Start();
			LOG_INFO("New connection comming.");
		}
	);
}

void HttpServer::Start()
{
    tcp_server_.Start();
}

void HttpServer::AppendSession(std::int64_t session_id, const std::shared_ptr<HttpSession> &session)
{
	std::lock_guard<std::mutex> lock(session_mutex_); 
    sessions_.emplace(session_id, session);
}

void HttpServer::RemoveSession(std::int64_t session_id)
{
	std::lock_guard<std::mutex> lock(session_mutex_);
	if (sessions_.find(session_id) != sessions_.end()) {
		sessions_.erase(session_id);
	}
}

void HttpServer::Stop()
{
    tcp_server_.Stop();
}
