/// @file server.h
/// @brief 服务器类
/// @author Jyang.
/// @date 2026-1-17
/// @version 1.0

#pragma once
#include <acceptor.h>
#include <unordered_map>
#include <iostream>
#include <unordered_set>

namespace jl
{
    class Server : public std::enable_shared_from_this<Server>
    {
    public:
        Server(asio::io_context &ioct, const std::string &ip, unsigned short port);

        /// @brief 启动服务器
        /// @param thread_cnt 线程数量
        void Start(std::size_t thread_cnt = std::thread::hardware_concurrency() * 2);

        /// @brief 停止服务器
        void Stop();

        asio::io_context& GetIoContext();

        /// @brief 设置连接建立回调函数
        /// @param callback 连接建立回调函数
        void SetConnEstablishCallback(const ConnEstablishCallback &callback);
        
        ~Server();
        
        /// @brief 异步接受SIGINT停止信号
        void DoAwaitStop();

        std::unordered_set <std::shared_ptr<BaseConnection>> conn_set_;

    private:

        void WaitSignal();

    private:
        std::atomic<bool> stop_;
        asio::io_context &ioct_;
        std::shared_ptr<Acceptor> acceptor_;
        asio::signal_set signals_;
        std::vector<std::unique_ptr<std::thread>> io_threads_;
        // std::unordered_map<int, std::function<void(int)>> sig_handlers_;
    };
}