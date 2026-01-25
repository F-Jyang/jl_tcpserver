#include "ssl_connction.h"
#include <logger.h>
#include <global.h>

    #if 0
namespace jl
{

	SslConnection::SslConnection(asio::io_context& ioct, Socket&& socket) :
		BaseConnection(ioct),
		ssl_socket_(std::move(socket), jl::GetSslContext())
	{
	}

	void SslConnection ::Start()
	{
		shutdown_ = false;
		auto self = shared_from_this();
		PostTask([=]()
			{ this->ssl_socket_.async_handshake(ssl::stream_base::server,
				[self](const std::error_code& ec)
				{
					self->OnHandshake(ec);
				}
			);
			}
		);
	}

	ssl::stream<net::socket> &SslConnection::GetSocket()
	{
		return ssl_socket_;
	}

	void SslConnection::Read(std::size_t max_bytes)
	{
		read_buffer_.EnableWrite(max_bytes);
		auto self = shared_from_this();
		PostTask([=]()
			{
				this->ssl_socket_.async_read_some(asio::buffer(this->read_buffer_.End(), max_bytes),
					[=](const std::error_code& ec, size_t bytes_transferred)
					{
						if (!ec) {
							read_buffer_.AppendEnd(bytes_transferred);
						}
						self->OnRead(ec, bytes_transferred);
					}
				);
			}
		);
	}

	void SslConnection::ReadUntil(const std::string& sep, std::size_t max_bytes)
	{
		assert(false);
		read_buffer_.EnableWrite(max_bytes);
		auto self = shared_from_this();
		//PostTask(
		//	[=]()
		//	{
		//		asio::async_read_until(this->ssl_socket_, asio::dynamic_buffer(this->read_buffer_.buffer_, max_bytes), sep,
		//			[=](const std::error_code& ec, size_t bytes_transferred)
		//			{
		//				self->OnRead(ec, bytes_transferred);
		//			}
		//		);
		//	}
		//);

	}

	void SslConnection::Write(const void *data, std::size_t n)
	{
		if (n <= 0)
		{
			return;
		}
		write_buffer_.Append(data, n);
		auto self = shared_from_this();
		PostTask([=]()
			{
				this->ssl_socket_.async_write_some(asio::buffer(this->write_buffer_.Start(), this->write_buffer_.Size()),
					[=](const std::error_code& ec, size_t bytes_transferred)
					{
						if (!ec) {
							write_buffer_.AppendStart(bytes_transferred);
						}
						self->OnWrite(ec, bytes_transferred);
					}
				);
			}
		);
	}

	void SslConnection::Write(const std::string &data)
	{
		Write(data.c_str(), data.size());
	}

	void SslConnection::Close()
	{
		bool expected = false;
		if (shutdown_.compare_exchange_strong(expected, true))
		{
			std::error_code ec;
			timer_.cancel();
			// 不对是否异常，都关闭连接。先shutdown，再close
			ssl_socket_.lowest_layer().shutdown(net::socket::shutdown_both, ec);
			ssl_socket_.lowest_layer().close(ec);
			if (conn_close_callback_)
			{
				conn_close_callback_(shared_from_this());
			}
		}
	}

	SslConnection::~SslConnection()
	{
		// std::cout << "SslConnection destructor called" << std::endl;
		Close();
	}

	void SslConnection::OnRead(const std::error_code &ec, size_t bytes_transferred)
	{
		if (!ec)
		{
			timer_.cancel();
			if (message_comming_callback_)
			{
				message_comming_callback_(shared_from_this(), read_buffer_, bytes_transferred);
			}
			SetTimeout(timeout_);
		}
		else {
			if (ec != asio::error::eof) {
				std::error_code ignore;
				auto remote = ssl_socket_.lowest_layer().remote_endpoint(ignore);
				LOG_ERROR("{}:{} OnRead fail, message:{}", remote.address().to_string(), remote.port(), ec.message());
			};
			Close();
		}
	}

	void SslConnection::OnWrite(const std::error_code &ec, size_t bytes_transferred)
	{
		if (!ec)
		{
			timer_.cancel();
			if (write_finish_callback_)
			{
				write_finish_callback_(shared_from_this(), bytes_transferred);
			}
			SetTimeout(timeout_);
		}
		else {
			std::error_code ignore;
			auto remote = ssl_socket_.lowest_layer().remote_endpoint(ignore);
			LOG_ERROR("{}:{} OnWrite message:{}", remote.address().to_string(), remote.port(), ec.message());
			Close();
		}
	}

	void SslConnection::OnTimeout(const std::error_code &ec)
	{
		if (ec == asio::error::operation_aborted) // 定时器调用 expire 重置
		{
			return;
		}
		else if (!ec) // 正常超时
		{
			LOG_INFO("OnTimeout:{}", ec.message());
			if (conn_timeout_callback_)
			{
				conn_timeout_callback_(shared_from_this());
			}
		}
		else // 其他错误
		{
			auto remote = ssl_socket_.lowest_layer().remote_endpoint();
			LOG_ERROR("{}:{} OnTimeout fail:{}", remote.address().to_string(), remote.port(), ec.message());
			Close();
		}
	}

	void SslConnection::OnHandshake(const std::error_code &ec)
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
				auto remote = ssl_socket_.lowest_layer().remote_endpoint(ignore);
				LOG_ERROR("{}:{} OnHandshake fail, message:{}", remote.address().to_string(), remote.port(), ec.message());
			}
			Close();
		}
	}

}
    #endif