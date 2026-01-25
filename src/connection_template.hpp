/// @file connection_template.hpp
/// @brief 连接类模板
/// @author Jyang.
/// @date 2026-1-17
/// @version 1.0

#pragma once
#include <base_connection.h>
#include <global.h>
#include <queue>

namespace jl
{
    namespace detail {
        template<typename SocketType>
        struct SocketTypeTraits {

            static net::endpoint GetEndpoint(SocketType& socket);

            static void Close(SocketType& socket);
        };
        template<typename SocketType>
        inline net::endpoint SocketTypeTraits<SocketType>::GetEndpoint(SocketType& socket)
        {
            std::error_code ignore;
            return socket.remote_endpoint(ignore);
        }
        template<typename SocketType>
        inline void SocketTypeTraits<SocketType>::Close(SocketType& socket)
        {
			std::error_code ignore;
			socket.shutdown(Socket::shutdown_both, ignore);
			socket.close(ignore); // 文档要求: call shutdown() before closing the socket，否则可能会提示非法套接字，好像就算先shutdown也可能会
        }

        template<>
        struct SocketTypeTraits<SSLSocket> {
			static net::endpoint GetEndpoint(SSLSocket& stream) {
				std::error_code ignore;
				return stream.lowest_layer().remote_endpoint(ignore);
			}

			static void Close(SSLSocket& stream) {
				std::error_code ignore;
				stream.lowest_layer().shutdown(Socket::shutdown_both, ignore);
				stream.lowest_layer().close(ignore);
			}
        };
    }


    template<typename SocketType>
    class ConnectionTemplate : public BaseConnection
    {
    public:
		ConnectionTemplate(asio::io_context& ioct, Socket&& socket, std::size_t buffer_max_size = kDefaultBufferMaxSize);
		
        ConnectionTemplate(const ConnectionTemplate &) = delete; // 禁用拷贝构造函数

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

        SocketType ExtractSocket() &&;

        ~ConnectionTemplate();

    protected:

		void DoWrite() override;

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
        SocketType socket_;
    };

	template<typename SocketType>
	inline ConnectionTemplate<SocketType>::ConnectionTemplate(asio::io_context& ioct, Socket&& socket, std::size_t buffer_max_size) :
		BaseConnection(ioct, buffer_max_size),
		socket_(std::move(socket))
	{
	}

	template<>
	inline ConnectionTemplate<SSLSocket>::ConnectionTemplate(asio::io_context& ioct, Socket&& socket, std::size_t buffer_max_size) :
		BaseConnection(ioct, buffer_max_size),
		socket_(std::move(socket), GetSslContext())
	{
	}

	template<typename SocketType>
	inline ConnectionTemplate<SocketType>::~ConnectionTemplate()
	{
		Close();
	}

	
    template<typename SocketType>
    inline void ConnectionTemplate<SocketType>::Start()
    {
		Read();
    }

	template<>
	inline void ConnectionTemplate<SSLSocket>::Start() {
		auto self = shared_from_this();
		PostTask([=]() {
			this->socket_.async_handshake(ssl::stream_base::server,
				asio::bind_executor(io_strand_, [=](const std::error_code& ec) {
					self->OnHandshake(ec);
					})
			);
			}
		);
	}

    template<typename SocketType>
    inline void ConnectionTemplate<SocketType>::Read(std::size_t max_bytes)
    {
		auto self = shared_from_this();
		PostTask([=]() {
			//LOG_INFO("In strand: {}", io_strand_.running_in_this_thread() ? "true" : "false");  // for debug，应该true
			bool expected = false;
			if (read_in_progress_.compare_exchange_strong(expected, true)) {
				asio::async_read(this->socket_, this->read_buffer_, asio::transfer_at_least(1),
					asio::bind_executor(io_strand_, [=](const std::error_code& ec, size_t bytes_transferred)
						{
							/*if (!ec) {
								this->read_buffer_.commit(bytes_transferred); // note: async_read 会 commit
							}*/
							bool expected = true;
							read_in_progress_.compare_exchange_strong(expected, false);
							self->OnRead(ec, bytes_transferred);
						}
					)
				);
			}
			}
		);
    }

    template<typename SocketType>
    inline void ConnectionTemplate<SocketType>::ReadUntil(const std::string& sep, std::size_t max_bytes)
	{
		auto self = shared_from_this();
		PostTask([=]() {
			//buffer_.prepare(max_bytes); // read_util不需要手动prepare
			bool expected = false;
			if (read_in_progress_.compare_exchange_strong(expected, true)) {
				asio::async_read_until(this->socket_, this->read_buffer_, sep,
					asio::bind_executor(io_strand_, [=](const std::error_code& ec, size_t bytes_transferred)
						{
							bool expected = true;
							read_in_progress_.compare_exchange_strong(expected, false);
							self->OnRead(ec, bytes_transferred);
						}
					)
				);
			}
			}
		);
		/*io_strand_.wrap([=](const std::error_code& ec, size_t bytes_transferred) // asio::io_context::strand::wrap、post都是废弃api
			{
				bool expected = true;
				read_in_progress_.compare_exchange_strong(expected, false);
				self->OnRead(ec, bytes_transferred);
			}
		)*/ 
    }

    template<typename SocketType>
	inline void ConnectionTemplate<SocketType>::Write(const void* data, std::size_t n)
	{
		auto self = shared_from_this();
		std::string copy(static_cast<const char*>(data), n);
		PostTask([=]() {
			const bool write_in_progress = !this->send_queue_.empty();
			this->send_queue_.emplace(copy);
			if (!write_in_progress) {
				self->DoWrite();
			}}
		);
	}

	template<typename SocketType>
	inline void ConnectionTemplate<SocketType>::DoWrite()
	{
		std::size_t n = this->send_queue_.front().size();
		auto self = shared_from_this();
		asio::async_write(socket_,
			asio::buffer(this->send_queue_.front()),
			asio::transfer_exactly(n),
			asio::bind_executor(io_strand_, [=](const std::error_code& ec, size_t bytes_transferred)
				{
					self->OnWrite(ec, bytes_transferred);
					if (!ec) {
						this->send_queue_.pop();
						if (!this->send_queue_.empty()) {
							self->DoWrite();
						}
					}
				}
			)
		);

	}


    template<typename SocketType>
    inline void ConnectionTemplate<SocketType>::Write(const std::string& data)
    {
		Write(data.c_str(), data.size());
    }

    template<typename SocketType>
    inline void ConnectionTemplate<SocketType>::Close()
    {
		ConnectionState expected = ConnectionState::kActived;
		if (state_.compare_exchange_strong(expected, ConnectionState::kClosed))
		{
			std::error_code ignore;
            detail::SocketTypeTraits<SocketType>::Close(socket_);
			if (conn_close_callback_)
			{
				conn_close_callback_(shared_from_this());
			}
		}
    }

    template<typename SocketType>
    inline SocketType ConnectionTemplate<SocketType>::ExtractSocket()&&
    {
        return std::move(socket_);
    }

    template<typename SocketType>
    inline void ConnectionTemplate<SocketType>::OnRead(const std::error_code& ec, size_t bytes_transferred)
	{
		if (!ec)
		{
			ConstBuffer buffer = this->read_buffer_.data();
			if (message_comming_callback_)
			{
				message_comming_callback_(shared_from_this(), buffer);
			}
			this->read_buffer_.consume(bytes_transferred); // note: 手动消费掉数据，如果不手动消费需要使用 std::ostream.read 的形式消费掉
			//buffer = this->read_buffer_.data();			
		}
		else
		{
			if (ec != asio::error::eof) {
				std::error_code ignore;
				auto remote = detail::SocketTypeTraits<SocketType>::GetEndpoint(socket_);
				LOG_ERROR("{}:{} OnRead message:{}", remote.address().to_string(), remote.port(), ec.message());
			}
			Close();
		}
    }

	// TODO: OnReadUntil，需要干掉最后的sep

    template<typename SocketType>
    inline void ConnectionTemplate<SocketType>::OnWrite(const std::error_code& ec, size_t bytes_transferred)
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
			auto remote = detail::SocketTypeTraits<SocketType>::GetEndpoint(socket_);
			LOG_ERROR("{}:{} OnWrite message:{}", remote.address().to_string(), remote.port(), ec.message());
			Close();
		}
    }

    template<typename SocketType>
    inline void ConnectionTemplate<SocketType>::OnTimeout(const std::error_code& ec)
    {

		if (ec == asio::error::operation_aborted) // 定时器调用 expire 重置
		{
			return;
		}
		else if (!ec) // 正常超时
		{
			//if()
			if (conn_timeout_callback_)
			{
				conn_timeout_callback_(shared_from_this());
			}
		}
		else // 其他错误
		{
			std::error_code ignore;
			auto remote = detail::SocketTypeTraits<SocketType>::GetEndpoint(socket_);
			LOG_ERROR("{}:{} OnTimeout message:{}", remote.address().to_string(), remote.port(), ec.message());
			Close();
		}
    }

    template<typename SocketType>
    inline void ConnectionTemplate<SocketType>::OnHandshake(const std::error_code& ec)
    {
		if (!ec) {
			if (handshake_callback_)
			{
				handshake_callback_(shared_from_this());
			}
		}
		else {
			if (ec != asio::error::eof) {
				std::error_code ignore;
				auto remote = detail::SocketTypeTraits<SocketType>::GetEndpoint(socket_);
				LOG_ERROR("{}:{} OnHandshake fail, message:{}", remote.address().to_string(), remote.port(), ec.message());
			}
			Close();
		}
    }

	using Connection = ConnectionTemplate<Socket>;
	using SSLConnection = ConnectionTemplate<SSLSocket>;
}