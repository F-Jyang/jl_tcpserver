/// @file connection.h
/// @brief 连接类，用于管理与客户端的通信
/// @author Jyang.
/// @date 2026-1-17
/// @version 1.0

#pragma once
#include <base_connection.h>

    #if 0
namespace jl
{
    class Connection : public BaseConnection
    {
    public:
        Connection(asio::io_context &ioct, net::socket &&socket);

        Connection(const Connection &) = delete; // 禁用拷贝构造函数

        /// @brief 启动连接，开始异步读取数据
        void Start() override;

        /// @brief 异步读取数据
        /// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
        void Read(std::size_t max_bytes = kDefaultMaxReadBytes) override;

		/// @brief 异步读取数据直到读到sep，或读取到最大字节max_bytes
        /// @param sep 分隔符
	    /// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
        void ReadUntil(const std::string& sep, std::size_t max_bytes = kDefaultMaxReadBytes) override;

        /// @brief 异步写入数据
        /// @param data 数据指针
        /// @param n 数据字节数
        void Write(const void *data, std::size_t n) override;

        /// @brief 异步写入数据
        /// @param data 数据字符串
        void Write(const std::string &data) override;

        /// @brief 关闭连接
        void Close() override;

        net::socket ExtractSocket() &&;

        ~Connection();

    protected:
        /// @brief 处理读取完成事件
        /// @param ec 错误码
        /// @param bytes_transferred 实际读取字节数
        void OnRead(const std::error_code &ec, size_t bytes_transferred) override;

        /// @brief 处理写入完成事件
        /// @param ec 错误码
        /// @param bytes_transferred 实际写入字节数
        void OnWrite(const std::error_code &ec, size_t bytes_transferred) override;

        /// @brief 处理超时事件
        /// @param ec 错误码
        void OnTimeout(const std::error_code &ec) override;

    /// @brief 处理握手完成事件
    /// @param ec 错误码
        void OnHandshake(const std::error_code& ec) override;

    private:
        // std::uint64_t id_;
        net::socket socket_;
    };
}
    #endif