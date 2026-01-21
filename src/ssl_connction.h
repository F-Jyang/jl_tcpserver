#pragma once

#include <base_connection.h>
#include <asio/ssl.hpp>

namespace jl
{
    class SslConnection : public BaseConnection
    {
    public:
        SslConnection(asio::io_context& ioct, asio::ssl::context &ssl_context, Socket&& socket);
        
        /// @brief 启动连接，开始异步读取数据
        void Start();

        /// @brief 获取连接的 socket 对象
        /// @return 连接的 socket 对象引用
        ssl::stream<net::socket> &GetSocket();

        /// @brief 异步读取数据
        /// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
        void Read(std::size_t max_bytes = kDefaultMaxReadBytes) override;

        /// @brief 异步写入数据
        /// @param data 数据指针
        /// @param n 数据字节数
        void Write(const void *data, std::size_t n) override;

        /// @brief 异步写入数据
        /// @param data 数据字符串
        void Write(const std::string &data) override;

        /// @brief 关闭连接
        void Close() override;

        ~SslConnection();

    protected:
        /// @brief 处理读取完成事件
        /// @param ec 错误码
        /// @param bytes_transferred 实际读取字节数
        void OnRead(const std::error_code &ec, size_t bytes_transferred);

        /// @brief 处理写入完成事件
        /// @param ec 错误码
        /// @param bytes_transferred 实际写入字节数
        void OnWrite(const std::error_code &ec, size_t bytes_transferred);

        /// @brief 处理超时事件
        /// @param ec 错误码
        void OnTimeout(const std::error_code &ec);

        void OnHandshake(const std::error_code &ec);

    private:
        // std::uint64_t id_;
        asio::ssl::stream<net::socket> ssl_socket_;
    };

};