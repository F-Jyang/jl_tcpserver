#include "connection.h"
#include <logger.h>
#include <global.h>

namespace jl {
	class Connection : public IConnection {
	public:
		Connection(net::socket&& socket, std::size_t max_buffer_size = kDefaultBufferMaxSize) :
			IConnection(max_buffer_size),
			socket_(std::move(socket))
		{
		}

		/// @brief 握手
		void Handshake() 
		{
			auto self = shared_from_this();
			if (this->handshake_callback_) {
				this->handshake_callback_(shared_from_this());
			}
		}

		/// @brief 异步读取数据
		/// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
		void Read()
		{
			bool expected = false;
			if (read_in_progress_.compare_exchange_strong(expected, true)) {
				auto self = shared_from_this();
				asio::async_read(this->socket_, read_buffer_, asio::transfer_at_least(1),
					[self, this](const std::error_code& ec, std::size_t bytes_transferred)
					{
						if (state_ != ConnectionState::kClosed) {
							bool expected = true;
							read_in_progress_.compare_exchange_strong(expected, false);
							this->OnRead(ec, bytes_transferred, 0);
						}
					}
				);
			}
		}

		void ReadN(std::size_t exactly_bytes)
		{
			bool expected = false;
			if (read_in_progress_.compare_exchange_strong(expected, true)) {
				auto self = shared_from_this();
				asio::async_read(socket_, read_buffer_, asio::transfer_exactly(exactly_bytes),
					[self, this](const std::error_code& ec, std::size_t bytes_transferred)
					{
						if (this->state_ != ConnectionState::kClosed)
						{
							bool expected = true;
							read_in_progress_.compare_exchange_strong(expected, false);
							this->OnRead(ec, bytes_transferred, 0);
						}
					}
				);
			}
		}

		void ReadUntil(const std::string& sep)
		{
			bool expected = false;
			if (read_in_progress_.compare_exchange_strong(expected, true)) {
				auto self = shared_from_this();
				// note: read_until 读取的是包含sep的数据，而不是以sep为结束的数据。因此读取的数据量可能会更多
				//		但是bytes_transfferred 表示的是第一个sep出现索引，所以可以使用bytes_transfferred来表示读取的长度
				asio::async_read_until(socket_, read_buffer_, sep,
					[self, this, sep](const std::error_code& ec, size_t bytes_transferred)
				{
					if (this->state_ != ConnectionState::kClosed)
					{
						bool expected = true;
						read_in_progress_.compare_exchange_strong(expected, false);
						this->OnRead(ec, bytes_transferred, sep.size());
					}
				}
				);
			}
		}

		const asio::any_io_executor& GetExecutor()
		{
			return socket_.get_executor();
		}


		/// @brief 异步写入数据
		/// @param data 数据指针
		/// @param n 数据字节数
		void Write(const void* data, std::size_t n)
		{
			auto self = shared_from_this();
			std::string copy(static_cast<const char*>(data), n);
			asio::post(GetExecutor(), // 保证send_queue线程安全
				[self, this, copy = std::move(copy)]() {
				const bool write_in_progress = !this->send_queue_.empty();
				this->send_queue_.emplace(copy);
				if (!write_in_progress) {
					this->DoWrite();
				}
			}
			);
		}

		/// @brief 异步写入数据
		/// @param data 数据字符串
		void Write(const std::string& data)
		{
			Write(data.c_str(), data.size());
		}

		/// @brief 关闭连接
		void Close()
		{
			ConnectionState expected = ConnectionState::kActived;
			if (state_.compare_exchange_strong(expected, ConnectionState::kClosed)) {
				std::error_code ignore;
				socket_.shutdown(net::socket::shutdown_both, ignore);
				socket_.close(ignore); // 文档要求: call shutdown() before closing the socket，否则可能会提示非法套接字，好像就算先shutdown也可能会
				if (this->conn_close_callback_) {
					this->conn_close_callback_(shared_from_this());
				}
			}
		}

		net::endpoint GetRemoteEndpoint() const
		{
			std::error_code ignore;
			return socket_.remote_endpoint(ignore);
		}

		net::endpoint GetLocalEndpoint() const
		{
			std::error_code ignore;
			return socket_.local_endpoint(ignore);
		}

		~Connection()
		{
			LOG_DEBUG("Connection destruct");
		}
	private:
		void DoWrite()
		{
			//std::string data = this->send_queue_.front();
			auto self = shared_from_this();
			asio::async_write(socket_, asio::buffer(this->send_queue_.front()), asio::transfer_exactly(this->send_queue_.front().size()),
				[self, this](const std::error_code& ec, size_t bytes_transferred)
				{
					if (state_ != ConnectionState::kClosed) // 连接已断开
					{
						this->send_queue_.pop();
						this->OnWrite(ec, bytes_transferred);
						if (!ec) {
							if (!this->send_queue_.empty()) {
								this->DoWrite();
							}
						}
					}
				}
			);
		}

		/// @brief 处理读取完成事件
		/// @param ec 错误码
		/// @param bytes_transferred 实际读取字节数
		void OnRead(const std::error_code& ec, size_t bytes_transferred, std::size_t sep_len)
		{
			{
				if (!ec)
				{
					std::istream is(&this->read_buffer_);
					std::string result(bytes_transferred, ' ');
					is.read(&result[0], bytes_transferred);
					if (sep_len > 0) {
						result.resize(bytes_transferred - sep_len);
					}
					//ConstBuffer buffer = this->read_buffer_.data();
					if (message_comming_callback_)
					{
						message_comming_callback_(shared_from_this(), result);
					}
					//this->read_buffer_.consume(bytes_transferred); // note: 手动消费掉数据，如果不手动消费需要使用 std::ostream.read 的形式消费掉
					//buffer = this->read_buffer_.data();			
				}
				else
				{
					if (ec != asio::error::eof) {
						std::error_code ignore;
						auto remote = GetRemoteEndpoint();
						LOG_ERROR("{}:{} OnRead message:{}", remote.address().to_string(), remote.port(), ec.message());
					}
					Close();
				}
			}
		}

		/// @brief 处理写入完成事件
		/// @param ec 错误码
		/// @param bytes_transferred 实际写入字节数
		void OnWrite(const std::error_code& ec, size_t bytes_transferred)
		{
			if (!ec)
			{
				if (write_finish_callback_)
				{
					write_finish_callback_(shared_from_this(), bytes_transferred);
				}
			}
			else {
				std::error_code ignore;
				auto remote = GetRemoteEndpoint();
				LOG_ERROR("{}:{} OnWrite message:{}", remote.address().to_string(), remote.port(), ec.message());
				Close();
			}
		}

		///// @brief 握手完成事件
		///// @param ec 错误码
		//void OnHandshake(const std::error_code& ec);

	private:
		net::socket socket_;
	};

	class SSLConnection : public IConnection {
	public:
		SSLConnection(net::socket&& socket, std::size_t max_buffer_size = kDefaultBufferMaxSize) :
			IConnection(max_buffer_size),
			socket_(std::move(socket), Global::Instance().GetSSLContext())
		{
		}


		/// @brief 握手
		void Handshake()
		{
			auto self = shared_from_this();
			socket_.async_handshake(ssl::stream_base::server,
				[self, this](const std::error_code& ec) {
					this->OnHandshake(ec);
				}
			);
		}

		/// @brief 异步读取数据
		/// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
		void Read()
		{
			bool expected = false;
			if (read_in_progress_.compare_exchange_strong(expected, true)) {
				auto self = shared_from_this();
				asio::async_read(this->socket_, read_buffer_, asio::transfer_at_least(1),
					[self, this](const std::error_code& ec, std::size_t bytes_transferred)
					{
						if (state_ != ConnectionState::kClosed) {
							bool expected = true;
							read_in_progress_.compare_exchange_strong(expected, false);
							this->OnRead(ec, bytes_transferred, 0);
						}
					}
				);
			}
		}

		void ReadN(std::size_t exactly_bytes)
		{
			bool expected = false;
			if (read_in_progress_.compare_exchange_strong(expected, true)) {
				auto self = shared_from_this();
				asio::async_read(socket_, read_buffer_, asio::transfer_exactly(exactly_bytes),
					[self, this](const std::error_code& ec, std::size_t bytes_transferred)
					{
						if (this->state_ != ConnectionState::kClosed)
						{
							bool expected = true;
							read_in_progress_.compare_exchange_strong(expected, false);
							this->OnRead(ec, bytes_transferred, 0);
						}
					}
				);
			}
		}

		void ReadUntil(const std::string& sep)
		{
			bool expected = false;
			if (read_in_progress_.compare_exchange_strong(expected, true)) {
				auto self = shared_from_this();
				// note: read_until 读取的是包含sep的数据，而不是以sep为结束的数据。因此读取的数据量可能会更多
				//		但是bytes_transfferred 表示的是第一个sep出现索引，所以可以使用bytes_transfferred来表示读取的长度
				asio::async_read_until(socket_, read_buffer_, sep,
					[self, this, sep = std::move(sep)](const std::error_code& ec, size_t bytes_transferred)
				{
					if (this->state_ != ConnectionState::kClosed)
					{
						bool expected = true;
						read_in_progress_.compare_exchange_strong(expected, false);
						this->OnRead(ec, bytes_transferred, sep.size());
					}
				}
				);
			}
		}

		const asio::any_io_executor& GetExecutor()
		{
			return socket_.lowest_layer().get_executor();
		}

		/// @brief 异步写入数据
		/// @param data 数据指针
		/// @param n 数据字节数
		void Write(const void* data, std::size_t n)
		{
			auto self = shared_from_this();
			std::string copy(static_cast<const char*>(data), n);
			asio::post(GetExecutor(), // 保证send_queue线程安全
				[self, this, copy = std::move(copy)]() {
				const bool write_in_progress = !this->send_queue_.empty();
				this->send_queue_.emplace(copy);
				if (!write_in_progress) {
					this->DoWrite();
				}
			}
			);
		}

		/// @brief 异步写入数据
		/// @param data 数据字符串
		void Write(const std::string& data)
		{
			Write(data.c_str(), data.size());
		}

		/// @brief 关闭连接
		void Close()
		{
			ConnectionState expected = ConnectionState::kActived;
			if (state_.compare_exchange_strong(expected, ConnectionState::kClosed)) {
				std::error_code ignore;
				auto self = shared_from_this();
				std::shared_ptr<bool> has_close = std::make_shared<bool>(false);
				std::shared_ptr<asio::steady_timer> handshake_timer = std::make_shared<asio::steady_timer>(GetExecutor());
				socket_.async_shutdown(
					[self, this, handshake_timer, has_close](const asio::error_code& ec) {
						if (*has_close)return;
						handshake_timer->cancel();
						if (ec && ec != asio::ssl::error::stream_truncated)  // shutdown failed!
						{
							LOG_DEBUG("SSL shutdown error: {}", ec.message());
						}
						std::error_code ignore;
						socket_.lowest_layer().close(ignore);
						if (this->conn_close_callback_) {
							this->conn_close_callback_(self);
						}
						LOG_DEBUG("SSL shutdown successful");
					}
				);
				handshake_timer->expires_after(std::chrono::seconds(3));
				handshake_timer->async_wait(
					[self, this, has_close](const asio::error_code& ec) {
						if (!ec) {
							LOG_DEBUG("SSL shutdown timeout");
							std::error_code ignore;
							this->socket_.lowest_layer().close(ignore);
							if (this->conn_close_callback_) {
								this->conn_close_callback_(self);
							}
							*has_close = true;
						}
					}
				);
			}
		}

		net::endpoint GetRemoteEndpoint() const
		{
			std::error_code ignore;
			return socket_.lowest_layer().remote_endpoint(ignore);
		}

		net::endpoint GetLocalEndpoint() const
		{
			std::error_code ignore;
			return socket_.lowest_layer().local_endpoint(ignore);
		}

		~SSLConnection()
		{
			LOG_DEBUG("SSLConnection destruct");
		}
	private:
		void DoWrite()
		{
			//std::string data = this->send_queue_.front();
			auto self = shared_from_this();
			asio::async_write(socket_, asio::buffer(this->send_queue_.front()), asio::transfer_exactly(this->send_queue_.front().size()),
				[self, this](const std::error_code& ec, size_t bytes_transferred)
				{
					if (state_ != ConnectionState::kClosed) // 连接已断开
					{
						this->send_queue_.pop();
						this->OnWrite(ec, bytes_transferred);
						if (!ec) {
							if (!this->send_queue_.empty()) {
								this->DoWrite();
							}
						}
					}
				}
			);
		}

		/// @brief 处理读取完成事件
		/// @param ec 错误码
		/// @param bytes_transferred 实际读取字节数
		void OnRead(const std::error_code& ec, size_t bytes_transferred, std::size_t sep_len)
		{
			if (!ec)
			{
				std::istream is(&this->read_buffer_);
				std::string result(bytes_transferred, ' ');
				is.read(&result[0], bytes_transferred);
				if (sep_len > 0) {
					result.resize(bytes_transferred - sep_len);
				}
				//ConstBuffer buffer = this->read_buffer_.data();
				if (message_comming_callback_)
				{
					message_comming_callback_(shared_from_this(), result);
				}
				//this->read_buffer_.consume(bytes_transferred); // note: 手动消费掉数据，如果不手动消费需要使用 std::ostream.read 的形式消费掉
				//buffer = this->read_buffer_.data();			
			}
			else
			{
				if (ec != asio::error::eof) {
					std::error_code ignore;
					auto remote = GetRemoteEndpoint();
					LOG_ERROR("{}:{} OnRead message:{}", remote.address().to_string(), remote.port(), ec.message());
				}
				Close();
			}
		}

		/// @brief 处理写入完成事件
		/// @param ec 错误码
		/// @param bytes_transferred 实际写入字节数
		void OnWrite(const std::error_code& ec, size_t bytes_transferred)
		{
			if (!ec)
			{
				if (write_finish_callback_)
				{
					write_finish_callback_(shared_from_this(), bytes_transferred);
				}
			}
			else
			{
				std::error_code ignore;
				auto remote = GetRemoteEndpoint();
				LOG_ERROR("{}:{} OnWrite message:{}", remote.address().to_string(), remote.port(), ec.message());
				Close();
			}
		}

		/// @brief 握手完成事件
		/// @param ec 错误码
		void OnHandshake(const std::error_code& ec)
		{
			if (!ec)
			{
				if (handshake_callback_)
				{
					handshake_callback_(shared_from_this());
				}
			}
			else
			{
				if (ec != asio::error::eof) {
					std::error_code ignore;
					auto remote = GetRemoteEndpoint();
					LOG_ERROR("{}:{} OnHandshake fail, message:{}", remote.address().to_string(), remote.port(), ec.message());
				}
				Close();
			}
		}

	private:
		ssl::stream<net::socket> socket_;
	};
}

std::shared_ptr<jl::IConnection> jl::MakeConnection(net::socket&& socket, std::size_t max_buffer_size)
{
	return std::make_shared<jl::Connection>(std::move(socket), max_buffer_size);
}

std::shared_ptr<jl::IConnection> jl::MakeSSLConnection(net::socket&& socket, std::size_t max_buffer_size)
{
	return std::make_shared<jl::SSLConnection>(std::move(socket), max_buffer_size);
}
