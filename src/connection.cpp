#include "connection.h"
#include <logger.h>
#include <socket.h>

jl::Connection::Connection(const std::shared_ptr<ISocket>& sockptr, std::size_t buffer_max_size) :
	socket_(sockptr),
	read_buffer_(buffer_max_size),
	read_in_progress_(false),
	state_(ConnectionState::kActived)
{
}

void jl::Connection::Start()
{
}

void jl::Connection::Handshake()
{
	auto self = shared_from_this();
	socket_->Handshake([=](const std::error_code& ec) {
		self->OnHandshake(ec);
		});
}

void jl::Connection::Read()
{
	auto self = shared_from_this();
	bool expected = false;
	if (read_in_progress_.compare_exchange_strong(expected, true)) {
		socket_->Read(this->read_buffer_,
			[=](const std::error_code& ec, std::size_t bytes_transferred)
			{
				if (state_ == ConnectionState::kClosed) // 连接已断开
				{
					return;
				}
				bool expected = true;
				read_in_progress_.compare_exchange_strong(expected, false);
				self->OnRead(ec, bytes_transferred, 0);
			}
		);
	}
}

void jl::Connection::ReadN(std::size_t exactly_bytes)
{
	auto self = shared_from_this();
	bool expected = false;
	if (read_in_progress_.compare_exchange_strong(expected, true)) {
		socket_->ReadN(exactly_bytes, this->read_buffer_,
			[=](const std::error_code& ec, std::size_t bytes_transferred)
			{
				if (state_ == ConnectionState::kClosed) // 连接已断开
				{
					return;
				}
				bool expected = true;
				read_in_progress_.compare_exchange_strong(expected, false);
				self->OnRead(ec, bytes_transferred, 0);
			}
		);
	}
}

void jl::Connection::ReadUntil(const std::string& sep)
{
	auto self = shared_from_this();
	bool expected = false;
	if (read_in_progress_.compare_exchange_strong(expected, true)) {
		// note: read_until 读取的是包含sep的数据，而不是以sep为结束的数据。因此读取的数据量可能会更多
		//		但是bytes_transfferred 表示的是第一个sep出现索引，所以可以使用bytes_transfferred来表示读取的长度
		socket_->ReadUntil(sep, this->read_buffer_,
			[=](const std::error_code& ec, size_t bytes_transferred)
			{
				if (state_ == ConnectionState::kClosed) // 连接已断开
				{
					return;
				}
				bool expected = true;
				read_in_progress_.compare_exchange_strong(expected, false);
				self->OnRead(ec, bytes_transferred, sep.size());
			}
		);
	}
}

void jl::Connection::Write(const void* data, std::size_t n)
{
	auto self = shared_from_this();
	std::string copy(static_cast<const char*>(data), n);
	asio::post(socket_->GetExecutor(), // 保证send_queue线程安全
		[=]() {
			const bool write_in_progress = !this->send_queue_.empty();
			this->send_queue_.emplace(copy);
			if (!write_in_progress) {
				self->DoWrite();
			}
		}
	);
}

void jl::Connection::Write(const std::string& data)
{
	Write(data.c_str(), data.size());
}

void jl::Connection::DoWrite()
{
	std::size_t n = this->send_queue_.front().size();
	auto self = shared_from_this();
	socket_->Write(
		this->send_queue_.front(),
		[=](const std::error_code& ec, size_t bytes_transferred)
		{
			if (state_ == ConnectionState::kClosed) // 连接已断开
			{
				return;
			}
			self->OnWrite(ec, bytes_transferred);
			if (!ec) {
				this->send_queue_.pop();
				if (!this->send_queue_.empty()) {
					self->DoWrite();
				}
			}
		}
	);
}

void jl::Connection::Close()
{
	auto self = shared_from_this();
	ConnectionState expected = ConnectionState::kActived;
	if (state_.compare_exchange_strong(expected, ConnectionState::kClosed)) {
		socket_->Close(
			[=]() {
				if (self->conn_close_callback_) {
					self->conn_close_callback_(self);
				}
			}
		);
	}
}

jl::net::endpoint jl::Connection::GetRemoteEndpoint() const
{
	return socket_->GetRemoteEndpoint();
}

jl::net::endpoint jl::Connection::GetLocalEndpoint() const
{
	return socket_->GetLocalEndpoint();
}

const asio::any_io_executor& jl::Connection::GetExecutor()
{
	return socket_->GetExecutor();
}

void jl::Connection::OnRead(const std::error_code& ec, size_t bytes_transferred, std::size_t sep_len)
{
	if (!ec)
	{
		std::istream is(&this->read_buffer_);
		std::string result(bytes_transferred, ' ');
		is.read(result.data(), bytes_transferred);
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

void jl::Connection::OnWrite(const std::error_code& ec, size_t bytes_transferred)
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

void jl::Connection::OnHandshake(const std::error_code& ec)
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
			auto remote = GetRemoteEndpoint();
			LOG_ERROR("{}:{} OnHandshake fail, message:{}", remote.address().to_string(), remote.port(), ec.message());
		}
		Close();
	}
}

std::shared_ptr<jl::Connection> jl::MakeConnection(net::socket&& socket, std::size_t max_buffer_size)
{
	return std::make_shared<jl::Connection>(jl::MakeSocket<jl::Socket>(std::move(socket)),max_buffer_size);
}

std::shared_ptr<jl::Connection> jl::MakeSSLConnection(net::socket&& socket, std::size_t max_buffer_size)
{
	return std::make_shared<jl::Connection>(jl::MakeSocket<jl::SSLSocket>(std::move(socket)), max_buffer_size);
}
