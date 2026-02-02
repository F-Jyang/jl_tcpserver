#include "http_session.h"

#include <http_server.h>
#include <util.h>

void HttpSession::Start()
{
    std::weak_ptr<HttpSession> weak = shared_from_this();
    conn_->SetHandshakeCallback(
        [=](const std::shared_ptr<jl::IConnection>& conn)
        {
            conn->ReadUntil("\r\n");
        });
    conn_->SetMessageCommingCallback(
        [=](const std::shared_ptr<jl::IConnection>& conn, const std::string& buffer)
        {
            auto self = weak.lock();
            if (self) {
                timer_->Cancel();
                if (state_ == State::kRequestLine)
                {
                    request_.request_line_ = buffer;
#ifdef _DEBUG
                    LOG_INFO("{}:{}> Request line: {}", remote_ip_, remote_port_, buffer);
#endif
                    state_ = State::kHeaders;
                    conn->ReadUntil("\r\n");
                }
                else if (state_ == State::kHeaders)
                {
#ifdef _DEBUG
                    LOG_INFO("{}:{}> Header: {}", remote_ip_, remote_port_, buffer);
#endif
                    if (buffer.size() > 0) // 读取到空行，头部读取完毕
                    {
                        std::string key = buffer.substr(0, buffer.find(":"));
                        std::string value = buffer.substr(buffer.find(":") + 2); // 跳过空格和冒号
                        request_.headers_[key] = value;
                        conn->ReadUntil("\r\n");
                    }
                    else
                    {
                        if (request_.headers_.find("Content-Length") != request_.headers_.end()) // 有Content-Length字段，读取body
                        {
                            state_ = State::kBody;
                            int content_length = std::stol(request_.headers_["Content-Length"]); // 获取Content-Length字段的值 fix: stol bug
                            conn->ReadN(content_length);                                // 读取body
                        }
                        // else if (headers_.find("Transfer-Encoding") != headers_.end()) // 有Transfer-Encoding字段，读取body (暂时不支持)
                        // {
                        // }
                        else // 没有Content-Length、Transfer-Encoding字段，说明没有request body
                        {
                            state_ = State::kDone;
                            std::string response = GetHttpResponse(request_);
                            conn->Write(response);
                        }
                    }
                }
            }
            else if (state_ == State::kBody)
            {
                state_ = State::kDone;
                request_.body_ = buffer;
#ifdef _DEBUG
                LOG_INFO("{}:{}> Request body: {}", remote_ip_, remote_port_, buffer);
#endif // DEBUG
                std::string response = GetHttpResponse(request_);
                conn->Write(response);
            }
            else
            {
                assert(false);
            }
            timer_->Wait(10000);
        });

    conn_->SetWriteFinishCallback([=](const std::shared_ptr<jl::IConnection>& conn, std::size_t bytes_transferred)
        {
            auto self = weak.lock();
            if (self) {
                timer_->Cancel();
                assert(state_ == State::kDone);
                state_ = State::kRequestLine;
                conn->ReadUntil("\r\n");
                timer_->Wait(10000);
                //LOG_INFO("Write finish: {}", bytes_transferred); 
            }
        });
    conn_->SetConnCloseCallback([=](const std::shared_ptr<jl::IConnection>& conn)
        {
            auto self = weak.lock();
            if (self) {
                auto server = server_.lock();
                if (server) {
                    LOG_INFO("{}:{}> Connection closed.", remote_ip_, remote_port_);
                    server->RemoveSession(this->session_id_);
                }
            }
        });
    timer_->SetCallback([=]()
        {
            auto self = weak.lock();
            if (self) {
                self->OnTimeout();
            };
        });
    timer_->Wait(10000);
    //conn_->ReadUntil("\r\n");
    conn_->Handshake();
}

HttpSession::~HttpSession()
{
    LOG_ERROR("HttpSession destruct");
}

void HttpSession::OnTimeout()
{
    LOG_ERROR("{}:{}> Connection timeout.", remote_ip_, remote_port_);
    conn_->Close();
}
