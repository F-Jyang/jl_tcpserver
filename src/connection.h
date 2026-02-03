/// @file connection.h
/// @brief 连接类
/// @author Jyang.
/// @date 2026-1-30
/// @version 1.0

#pragma once

#include <define.h>
#include <queue>

namespace jl
{
	constexpr std::size_t kDefaultMaxReadBytes = 2048;
	constexpr std::size_t kDefaultTimeout = 5 * 60;
	constexpr std::size_t kDefaultBufferMaxSize = 1024 * 4;

	enum class ConnectionState {
		kActived = 1,
		kClosing,
		kClosed,
	};

	enum class Option {
		RD_ONLY = 1,
		WR_ONLY = 1 << 1,
		RDWR = RD_ONLY + WR_ONLY,
	};

	inline bool operator&(Option opt1, Option opt2) {
		return static_cast<int>(opt1) & static_cast<int>(opt2);
	}

	inline Option operator|(Option opt1, Option opt2) {
		return static_cast<Option>(static_cast<int>(opt1) | static_cast<int>(opt2));
	}

	class IConnection : public std::enable_shared_from_this<IConnection> {
	public:
		IConnection(std::size_t max_buffer_size) :
			read_buffer_(max_buffer_size),
			read_in_progress_(false),
			state_(ConnectionState::kActived)
		{}

		/// @brief 握手
		virtual void Handshake() = 0;

		/// @brief 异步读取数据
		/// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
		virtual void Read() = 0;

		virtual void ReadN(std::size_t exactly_bytes) = 0; 

		virtual void ReadUntil(const std::string& sep) = 0;

		virtual const asio::any_io_executor& GetExecutor() = 0;

		/// @brief 异步写入数据
		/// @param data 数据指针
		/// @param n 数据字节数
		virtual void Write(const void* data, std::size_t n) = 0;

		/// @brief 异步写入数据
		/// @param data 数据字符串
		virtual void Write(const std::string& data) = 0;

		/// @brief 关闭连接
		virtual void Close() = 0;

		virtual net::endpoint GetRemoteEndpoint() const = 0;

		virtual net::endpoint GetLocalEndpoint() const = 0;

		/// @brief 设置写入完成回调函数
		/// @param callback 写入完成回调函数
		virtual void SetWriteFinishCallback(WriteFinishCallback callback) { write_finish_callback_ = callback; }

		/// @brief 设置写入完成回调函数
		/// @param callback 写入完成回调函数
		virtual void SetHandshakeCallback(HandshakeCallback callback) { handshake_callback_ = callback; }

		/// @brief 设置消息到达回调函数
		/// @param callback 消息到达回调函数
		virtual void SetMessageCommingCallback(MessageCommingCallback callback) { message_comming_callback_ = callback; }

		/// @brief 设置连接关闭回调函数
		/// @param callback 连接关闭回调函数
		virtual void SetConnCloseCallback(ConnCloseCallback callback) { conn_close_callback_ = callback; }

	protected:
		std::atomic<bool> read_in_progress_;
		std::atomic<ConnectionState> state_;
		asio::streambuf read_buffer_;
		std::queue<std::string> send_queue_;
		HandshakeCallback handshake_callback_;
		WriteFinishCallback write_finish_callback_;
		MessageCommingCallback message_comming_callback_;
		ConnCloseCallback conn_close_callback_;
	};

	// @brief 创建普通TCP连接
	// @param socket 
	// @param 连接最大读缓冲区
	std::shared_ptr<IConnection> MakeConnection(net::socket&& socket, std::size_t max_buffer_size = kDefaultBufferMaxSize);
	
	// @brief 创建SSL连接
	// @param socket 
	// @param 连接最大读缓冲区
	std::shared_ptr<IConnection> MakeSSLConnection(net::socket&& socket, std::size_t max_buffer_size = kDefaultBufferMaxSize);
}