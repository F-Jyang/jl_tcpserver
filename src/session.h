#pragma once
#include <memory>
#include <connection.h>

namespace jl
{
    class Session : public std::enable_shared_from_this<Session>
    {
    public:
        Session();
        ~Session();
    private:
        std::shared_ptr<Connection> connection_; // 连接对象
        std::string session_id_; // 会话ID        
    };

}