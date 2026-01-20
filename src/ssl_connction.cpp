#include "ssl_connction.h"
#include <logger.h>

namespace jl
{
	SslConnection::SslConnection(std::shared_ptr<Connection> conn, asio::ssl::context &ssl_context) : ioct_(conn->GetIoContext()),
																									  ssl_socket_(std::move(*conn).ExtractSocket(), ssl_context),
																									  io_strand_(conn->GetIoContext()),
																									  timer_(conn->GetIoContext()),
																									  shutdown_(true),
																									  timeout_(kDefaultTimeout)
	{
	}

	void SslConnection::SetTimeout(std::size_t secs)
	{
		timeout_ = secs;
		auto self = shared_from_this();
		timer_.expires_after(std::chrono::seconds(secs));
		timer_.async_wait(asio::bind_executor(io_strand_, [self](const std::error_code &ec)
											  { self->OnTimeout(ec); }));
	}

	void SslConnection ::Start()
	{
		shutdown_ = false;
		auto self = shared_from_this();
		PostTask([self]()
				 { self->ssl_socket_.async_handshake(ssl::stream_base::server, [self](const std::error_code &ec)
													 { self->OnHandshake(ec); }); });
	}

	ssl::stream<net::socket> &SslConnection::GetSocket()
	{
		return ssl_socket_;
	}

	void SslConnection::Read(std::size_t max_bytes)
	{
		read_buffer_.EnableWrite(max_bytes);
		auto self = shared_from_this();
		PostTask([self, max_bytes]()
				 { self->ssl_socket_.async_read_some(asio::buffer(self->read_buffer_.WriteStart(), max_bytes), [=](const std::error_code &ec, size_t bytes_transferred)
													 { self->OnRead(ec, bytes_transferred); }); });
		// io_strand_.post(
		//     [=]()
		//     {
		//         ssl_socket_.async_read_some(asio::buffer(read_buffer_.WriteStart(), max_bytes), [=](const std::error_code &ec, size_t bytes_transferred)
		//                                 { self->OnRead(ec, bytes_transferred); });
		//     },
		//     std::allocator<void>{});
	}

	void SslConnection::Write(const void *data, std::size_t n)
	{
		if (n <= 0)
		{
			return;
		}
		write_buffer_.Append(data, n);
		auto self = shared_from_this();
		PostTask([self]()
				 { self->ssl_socket_.async_write_some(asio::buffer(self->write_buffer_.ReadStart(), self->write_buffer_.Size()), [=](const std::error_code &ec, size_t bytes_transferred)
													  { self->OnWrite(ec, bytes_transferred); }); });
		// io_strand_.post(
		//     [=]()
		//     {
		//         self->ssl_socket_.async_write_some(asio::buffer(write_buffer_.ReadStart(), write_buffer_.Size()), [=](const std::error_code &ec, size_t bytes_transferred)
		//                                        { self->OnWrite(ec, bytes_transferred); });
		//     },
		//     std::allocator<void>{});
	}

	void SslConnection::Write(const std::string &data)
	{
		Write(data.c_str(), data.size());
	}

	void SslConnection::Close()
	{
		if (shutdown_)
			return;
		shutdown_ = true;
		timer_.cancel();
		auto self = shared_from_this();
		PostTask(
			[self]()
			{
				self->ssl_socket_.async_shutdown(
					[self](std::error_code ec)
					{
						if (ec == asio::error::eof) // 忽略short read
						{
							ec.clear();
						}
						// 不对是否异常，都关闭连接。先shutdown，再close
						self->ssl_socket_.lowest_layer().shutdown(net::socket::shutdown_both, ec);
						self->ssl_socket_.lowest_layer().close();
						if(self->conn_close_callback_)
						{
							self->conn_close_callback_(self);
						}
					});
			});
	}

	SslConnection::~SslConnection()
	{
		// std::cout << "SslConnection destructor called" << std::endl;
		Close();
	}

	void SslConnection::OnRead(const std::error_code &ec, size_t bytes_transferred)
	{
		if (ec)
		{
			std::error_code error;
			auto remote = ssl_socket_.lowest_layer().remote_endpoint(error);
			if (error)
			{
				LOG_ERROR("{}:{} OnRead fail:{}", remote.address().to_string(), remote.port(), error.message());
				Close();
				return;
			}
			else
			{
				LOG_ERROR("OnRead fail:{}", ec.message());
			}
			Close();
		}
		timer_.cancel();
		read_buffer_.AppendEnd(bytes_transferred);
		if (message_comming_callback_)
		{
			message_comming_callback_(shared_from_this(), bytes_transferred, read_buffer_);
		}
		SetTimeout(timeout_);
	}

	void SslConnection::OnWrite(const std::error_code &ec, size_t bytes_transferred)
	{
		if (ec)
		{
			std::error_code error;
			auto remote = ssl_socket_.lowest_layer().remote_endpoint(error);
			if (error)
			{
				LOG_ERROR("{}:{} OnWrite fail:{}", remote.address().to_string(), remote.port(), error.message());
				Close();
				return;
			}
			else
			{
				LOG_ERROR("OnWrite fail:{}", ec.message());
			}
			Close();
		}
		timer_.cancel();
		write_buffer_.AppendStart(bytes_transferred);
		if (write_finish_callback_)
		{
			write_finish_callback_(shared_from_this(), bytes_transferred);
		}
		SetTimeout(timeout_);
	}

	void SslConnection::OnTimeout(const std::error_code &ec)
	{
		if (ec == asio::error::operation_aborted) // 定时器调用 expire 重置
		{
			return;
		}
		else if (!ec) // 正常超时
		{
			auto remote = ssl_socket_.lowest_layer().remote_endpoint();
			LOG_INFO("{}:{} OnTimeout:{}", remote.address().to_string(), remote.port(), ec.message());
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
		if (ec)
		{
			std::error_code error;
			auto remote = ssl_socket_.lowest_layer().remote_endpoint(error);
			if (error)
			{
				LOG_ERROR("{}:{} OnHandshake fail:{}", remote.address().to_string(), remote.port(), error.message());
				Close();
				return;
			}
			else
			{
				LOG_ERROR("OnHandshake fail:{}", ec.message());
			}
			Close();
		}
		if (handshake_callback_)
		{
			handshake_callback_(shared_from_this());
		}
	}

	asio::io_context &SslConnection::GetIoContext()
	{
		return ioct_;
	}
}