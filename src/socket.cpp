#include "socket.h"
#include <logger.h>
#include <global.h>

namespace jl
{
	Socket::Socket(net::socket&& socket):
		socket_(std::move(socket))
	{
	}

	void Socket::Handshake(const SocketHandshakeCallback& callback)
	{
	}

	net::endpoint Socket::GetRemoteEndpoint() const
	{
		std::error_code ignore;
		return socket_.remote_endpoint(ignore);
	}
	
	net::endpoint Socket::GetLocalEndpoint() const
	{
		std::error_code ignore;
		return socket_.local_endpoint(ignore);
	}

	const asio::any_io_executor& Socket::GetExecutor()
	{
		return socket_.get_executor();
	}

	void Socket::Read(asio::streambuf& read_buffer, const SocketReadCallback& callback)
	{
		asio::async_read(this->socket_, read_buffer, asio::transfer_at_least(1), callback);
	}

	void Socket::ReadN(std::size_t exactly_bytes, asio::streambuf& read_buffer, const SocketReadCallback& callback)
	{
		asio::async_read(socket_, read_buffer, asio::transfer_exactly(exactly_bytes), callback);
	}

	void Socket::ReadUntil(const std::string& sep, asio::streambuf& read_buffer, const SocketReadCallback& callback)
	{
		asio::async_read_until(socket_, read_buffer, sep, callback);
	}

	void Socket::Write(const void* data, std::size_t n, const SocketWriteCallback& callback)
	{
		asio::async_write(socket_, asio::buffer(data, n), asio::transfer_exactly(n), callback);
	}

	void Socket::Write(const std::string& data, const SocketWriteCallback& callback)
	{
		this->Write(data.c_str(), data.size(), callback);
	}

	void Socket::Close(const SocketCloseCallback& callback)
	{
		std::error_code ignore;
		socket_.shutdown(net::socket::shutdown_both, ignore);
		socket_.close(ignore); // 文档要求: call shutdown() before closing the socket，否则可能会提示非法套接字，好像就算先shutdown也可能会
		callback();
	}
	
	SSLSocket::SSLSocket(net::socket&& socket):
		socket_(std::move(socket),GetSslContext())
	{
	}

	void SSLSocket::Handshake(const SocketHandshakeCallback& callback)
	{
		socket_.async_handshake(ssl::stream_base::server, callback);
	}

	void SSLSocket::Read(asio::streambuf& read_buffer, const SocketReadCallback& callback)
	{
		asio::async_read(this->socket_, read_buffer, asio::transfer_at_least(1), callback);
	}

	net::endpoint SSLSocket::GetRemoteEndpoint() const
	{
		std::error_code ignore;
		return socket_.lowest_layer().remote_endpoint(ignore);
	}
	
	net::endpoint SSLSocket::GetLocalEndpoint() const
	{
		std::error_code ignore;
		return socket_.lowest_layer().local_endpoint(ignore);
	}

	const asio::any_io_executor& SSLSocket::GetExecutor()
	{
		return socket_.lowest_layer().get_executor();
	}

	void SSLSocket::ReadN(std::size_t exactly_bytes, asio::streambuf& read_buffer, const SocketReadCallback& callback)
	{
		asio::async_read(socket_, read_buffer, asio::transfer_exactly(exactly_bytes), callback);
	}

	void SSLSocket::ReadUntil(const std::string& sep, asio::streambuf& read_buffer, const SocketReadCallback& callback)
	{
		asio::async_read_until(socket_, read_buffer, sep, callback);
	}

	void SSLSocket::Write(const void* data, std::size_t n, const SocketWriteCallback& callback)
	{
		asio::async_write(socket_, asio::buffer(data, n), asio::transfer_exactly(n), callback);
	}

	void SSLSocket::Write(const std::string& data, const SocketWriteCallback& callback)
	{
		this->Write(data.c_str(), data.size(), callback);
	}

	void SSLSocket::Close(const SocketCloseCallback& callback)
	{
		std::error_code ignore;
		auto self = shared_from_this();
		std::shared_ptr<asio::steady_timer> handshake_timer = std::make_shared<asio::steady_timer>(socket_.lowest_layer().get_executor());
		handshake_timer->async_wait(
			[=](const asio::error_code& ec) {
				if (!ec) {
					LOG_ERROR("SSL shutdown timeout");
					std::error_code ignore;
					this->socket_.lowest_layer().close(ignore);
					callback();
				}
			}
		);
		handshake_timer->expires_after(std::chrono::seconds(3));
		socket_.async_shutdown(
			[=](const asio::error_code& ec) {
				handshake_timer->cancel();
				if (ec && ec != asio::ssl::error::stream_truncated)  // shutdown failed!
				{
					LOG_ERROR("SSL shutdown error: {}", ec.message());
				}
				std::error_code ignore;
				socket_.lowest_layer().close(ignore);
				callback();
			}
		);
	}
}