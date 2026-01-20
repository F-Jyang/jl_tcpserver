#pragma once

#include <connection.h>
#include <asio/ssl.hpp>

namespace jl
{
    class SslConnection : public std::enable_shared_from_this<SslConnection>
    {
    public:
        SslConnection(std::shared_ptr<Connection> conn, asio::ssl::context &ssl_context);
        /// @brief 设置连接超时时间
        /// @param secs 超时时间，单位为秒，默认值为 kDefaultTimeout
        void SetTimeout(std::size_t secs = kDefaultTimeout);

        /// @brief 启动连接，开始异步读取数据
        void Start();

        /// @brief 获取连接的 socket 对象
        /// @return 连接的 socket 对象引用
        ssl::stream<net::socket> &GetSocket();

        /// @brief 设置写入完成回调函数
        /// @param callback 写入完成回调函数
        void SetWriteFinishCallback(SslWriteFinishCallback callback) { write_finish_callback_ = callback; }

        /// @brief 设置写入完成回调函数
        /// @param callback 写入完成回调函数
        void SetHandshakeCallback(SslHandshakeCallback callback) { handshake_callback_ = callback; }

        /// @brief 设置消息到达回调函数
        /// @param callback 消息到达回调函数
        void SetMessageCommingCallback(SslMessageCommingCallback callback) { message_comming_callback_ = callback; }

        /// @brief 设置连接关闭回调函数
        /// @param callback 连接关闭回调函数
        void SetConnCloseCallback(SslConnCloseCallback callback) { conn_close_callback_ = callback; }

        /// @brief 设置连接超时回调函数
        /// @param callback 连接超时回调函数
        void SetConnTimeoutCallback(SslConnTimeoutCallback callback) { conn_timeout_callback_ = callback; }

        /// @brief 异步读取数据
        /// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
        void Read(std::size_t max_bytes = kDefaultMaxReadBytes);

        /// @brief 异步写入数据
        /// @param data 数据指针
        /// @param n 数据字节数
        void Write(const void *data, std::size_t n);

        /// @brief 异步写入数据
        /// @param data 数据字符串
        void Write(const std::string &data);

        /// @brief 关闭连接
        void Close();

        asio::io_context &GetIoContext();

        ~SslConnection();

    private:
        /// @brief 异步执行任务，确保在 strand 中执行
        /// @tparam Function 可调用对象类型
        /// @tparam Allocator 分配器类型
        /// @param f 可调用对象
        /// @param a 分配器实例
        template <typename Function, typename Allocator = std::allocator<void>>
        void PostTask(Function &&f, const Allocator &a = std::allocator<void>{}) const;

        // TODO: 调用remote_endpoint需要处理异常

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
        bool shutdown_;
        asio::io_context &ioct_;
        asio::ssl::stream<net::socket> ssl_socket_;
        asio::io_context::strand io_strand_;
        Buffer read_buffer_;
        Buffer write_buffer_;
        asio::steady_timer timer_;
        std::size_t timeout_;
        SslHandshakeCallback handshake_callback_;
        SslWriteFinishCallback write_finish_callback_;
        SslMessageCommingCallback message_comming_callback_;
        SslConnCloseCallback conn_close_callback_;
        SslConnTimeoutCallback conn_timeout_callback_;
    };

    template <typename Function, typename Allocator>
    inline void SslConnection::PostTask(Function &&f, const Allocator &a) const
    {
        io_strand_.post(std::forward<Function>(f), a);
    }

};