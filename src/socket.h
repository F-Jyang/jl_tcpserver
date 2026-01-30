/// @file socket.h
/// @brief Socket类和SSLSocket类
/// @author Jyang.
/// @date 2026-1-30
/// @version 1.0

#pragma once
#include <isocket.h>

namespace jl
{
	class Socket : public ISocket {
	public:
		explicit Socket(net::socket&& socket);

		void Handshake(const SocketHandshakeCallback& callback) override;

		/// @brief 异步读取数据
		/// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
		void Read(asio::streambuf& read_buffer, const SocketReadCallback& callback) override;

		/// @brief 获取远端endpoint
		net::endpoint GetRemoteEndpoint() const override;

		// @brief 获取本地endpoint
		net::endpoint GetLocalEndpoint() const override;

		const asio::any_io_executor& GetExecutor() override;

		/// @brief 异步读取数据 exactly_bytes 个字节
		/// @param exactly_bytes 读取的字节数
		void ReadN(std::size_t exactly_bytes, asio::streambuf& read_buffer, const SocketReadCallback& callback) override;

		/// @brief 异步读取数据直到读到sep，或读取到最大字节max_bytes
		/// @param sep 分隔符
		/// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
		void ReadUntil(const std::string& sep, asio::streambuf& read_buffer, const SocketReadCallback& callback) override;

		/// @brief 异步写入数据
		/// @param data 数据指针
		/// @param n 数据字节数
		void Write(const void* data, std::size_t n, const SocketWriteCallback& callback) override;

		/// @brief 异步写入数据
		/// @param data 数据字符串
		void Write(const std::string& data, const SocketWriteCallback& callback) override;

		/// @brief 关闭连接
		void Close(const SocketCloseCallback& callback) override;

	private:
		net::socket socket_;
	};

	class SSLSocket : public ISocket {
	public:
		explicit SSLSocket(net::socket&& socket);

		void Handshake(const SocketHandshakeCallback& callback) override;

		/// @brief 异步读取数据
		/// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
		void Read(asio::streambuf& read_buffer, const SocketReadCallback& callback) override;

		/// @brief 获取远端endpoint
		net::endpoint GetRemoteEndpoint() const override;

		// @brief 获取本地endpoint
		net::endpoint GetLocalEndpoint() const override;

		const asio::any_io_executor& GetExecutor() override;

		/// @brief 异步读取数据 exactly_bytes 个字节
		/// @param exactly_bytes 读取的字节数
		void ReadN(std::size_t exactly_bytes, asio::streambuf& read_buffer, const SocketReadCallback& callback) override;

		/// @brief 异步读取数据直到读到sep，或读取到最大字节max_bytes
		/// @param sep 分隔符
		/// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
		void ReadUntil(const std::string& sep, asio::streambuf& read_buffer, const SocketReadCallback& callback) override;

		/// @brief 异步写入数据
		/// @param data 数据指针
		/// @param n 数据字节数
		void Write(const void* data, std::size_t n, const SocketWriteCallback& callback) override;

		/// @brief 异步写入数据
		/// @param data 数据字符串
		void Write(const std::string& data, const SocketWriteCallback& callback) override;

		/// @brief 关闭连接
		void Close(const SocketCloseCallback& callback) override;


	private:
		ssl::stream<net::socket> socket_;
	};

	template<typename SocketType>
	inline std::shared_ptr<ISocket> MakeSocket(net::socket&& socket) {
		return std::make_shared<SocketType>(std::move(socket));
	}
}