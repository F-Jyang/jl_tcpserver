/// @file isocket.h
/// @brief socket接口类
/// @author Jyang.
/// @date 2026-1-30
/// @version 1.0

#pragma once
#include <define.h>

namespace jl
{
  
	using SocketReadCallback = std::function<void(const std::error_code&, std::size_t)>;
	using SocketHandshakeCallback = std::function<void(const std::error_code&)>;
	using SocketWriteCallback = std::function<void(const std::error_code&, std::size_t)>;
	using SocketCloseCallback = std::function<void()>;

	class ISocket : public std::enable_shared_from_this<ISocket> {
	public:

		virtual void Handshake(const SocketHandshakeCallback& callback) = 0;

		/// @brief 获取远端endpoint
		virtual net::endpoint GetRemoteEndpoint() const = 0;

		// @brief 获取本地endpoint
		virtual net::endpoint GetLocalEndpoint() const = 0;

		virtual const asio::any_io_executor& GetExecutor() = 0;

		/// @brief 异步读取数据
		/// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
		virtual void Read(asio::streambuf& read_buffer, const SocketReadCallback& callback) = 0;

		/// @brief 异步读取数据 exactly_bytes 个字节
		/// @param exactly_bytes 读取的字节数
		virtual void ReadN(std::size_t exactly_bytes, asio::streambuf& read_buffer, const SocketReadCallback& callback) = 0;

		/// @brief 异步读取数据直到读到sep，或读取到最大字节max_bytes
		/// @param sep 分隔符
		/// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
		virtual void ReadUntil(const std::string& sep, asio::streambuf& read_buffer, const SocketReadCallback& callback) = 0;

		/// @brief 异步写入数据
		/// @param data 数据指针
		/// @param n 数据字节数
		virtual void Write(const void* data, std::size_t n, const SocketWriteCallback& callback) = 0;

		/// @brief 异步写入数据
		/// @param data 数据字符串
		virtual void Write(const std::string& data, const SocketWriteCallback& callback) = 0;

		/// @brief 关闭连接
		virtual void Close(const SocketCloseCallback& callback) = 0;
	};
}