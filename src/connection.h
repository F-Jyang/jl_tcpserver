/// @file connection.h
/// @brief 连接类
/// @author Jyang.
/// @date 2026-1-30
/// @version 1.0

#pragma once

#include <isocket.h>
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

	class Connection : public std::enable_shared_from_this<Connection> {
	public:
		Connection(const std::shared_ptr<ISocket>& sockptr, std::size_t buffer_max_size = kDefaultBufferMaxSize);

		void Start();

		/// @brief 握手
		void Handshake();

		/// @brief 异步读取数据
		/// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
		void Read();

		void ReadN(std::size_t exactly_bytes);

		void ReadUntil(const std::string& sep);

		const asio::any_io_executor& GetExecutor();

		/// @brief 异步写入数据
		/// @param data 数据指针
		/// @param n 数据字节数
		void Write(const void* data, std::size_t n);

		/// @brief 异步写入数据
		/// @param data 数据字符串
		void Write(const std::string& data);

		/// @brief 关闭连接
		void Close();

		net::endpoint GetRemoteEndpoint() const;

		net::endpoint GetLocalEndpoint() const;

		/// @brief 设置写入完成回调函数
		/// @param callback 写入完成回调函数
		void SetWriteFinishCallback(WriteFinishCallback callback) { write_finish_callback_ = callback; }

		/// @brief 设置写入完成回调函数
		/// @param callback 写入完成回调函数
		void SetHandshakeCallback(HandshakeCallback callback) { handshake_callback_ = callback; }

		/// @brief 设置消息到达回调函数
		/// @param callback 消息到达回调函数
		void SetMessageCommingCallback(MessageCommingCallback callback) { message_comming_callback_ = callback; }

		/// @brief 设置连接关闭回调函数
		/// @param callback 连接关闭回调函数
		void SetConnCloseCallback(ConnCloseCallback callback) { conn_close_callback_ = callback; }
		
		~Connection() {}
	private:
		void DoWrite();

		/// @brief 处理读取完成事件
		/// @param ec 错误码
		/// @param bytes_transferred 实际读取字节数
		void OnRead(const std::error_code& ec, size_t bytes_transferred, std::size_t sep_len);

		/// @brief 处理写入完成事件
		/// @param ec 错误码
		/// @param bytes_transferred 实际写入字节数
		void OnWrite(const std::error_code& ec, size_t bytes_transferred);

		/// @brief 握手完成事件
		/// @param ec 错误码
		void OnHandshake(const std::error_code& ec);

	private:
		std::shared_ptr<ISocket> socket_;
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
	std::shared_ptr<Connection> MakeConnection(net::socket&& socket, std::size_t max_buffer_size = kDefaultBufferMaxSize);
	
	// @brief 创建SSL连接
	// @param socket 
	// @param 连接最大读缓冲区
	std::shared_ptr<Connection> MakeSSLConnection(net::socket&& socket, std::size_t max_buffer_size = kDefaultBufferMaxSize);
}