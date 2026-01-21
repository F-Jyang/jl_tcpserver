#include "connection.h"
#include <iostream>
#include <logger.h>

namespace jl
{
    Connection::Connection(asio::io_context& ioct, net::socket&& socket) :
        BaseConnection(ioct),
        socket_(std::move(socket))
    {
    }

    void Connection::Start()
    {
        this->Read();
    }

    net::socket Connection::ExtractSocket() &&
    {
        return std::move(socket_);
    }

    void Connection::Read(std::size_t max_bytes)
    {
        read_buffer_.EnableWrite(max_bytes);
        auto self = shared_from_this();
        PostTask(
            [=]()
            { 
                this->socket_.async_read_some(asio::buffer(this->read_buffer_.WriteStart(), max_bytes),
                    [=](const std::error_code& ec, size_t bytes_transferred)
                    {
                        self->OnRead(ec, bytes_transferred);
                    }
                );
            }
        );
    }

    void Connection::Write(const void *data, std::size_t n)
    {
        if (n <= 0)
        {
            return;
        }
        write_buffer_.Append(data, n);
        auto self = shared_from_this();
        PostTask(
            [=]()
            {
                this->socket_.async_write_some(asio::buffer(this->write_buffer_.ReadStart(), this->write_buffer_.Size()),
                    [=](const std::error_code& ec, size_t bytes_transferred)
                    {
                        self->OnWrite(ec, bytes_transferred);
                    }
                );
            }
        );
    }

    void Connection::Write(const std::string &data)
    {
        Write(data.c_str(), data.size());
    }

    void Connection::Close()
    {
        bool expected = false;
        if (shutdown_.compare_exchange_strong(expected, true))
        {
            std::error_code ignore;
            timer_.cancel();
            socket_.shutdown(net::socket::shutdown_both, ignore);
            socket_.close(ignore); // 文档要求: call shutdown() before closing the socket，否则可能会提示非法套接字，好像就算先shutdown也可能会
            if (conn_close_callback_)
            {
                conn_close_callback_(shared_from_this());
            }
        }
    }

    Connection::~Connection()
    {
        Close();
    }

    void Connection::OnRead(const std::error_code &ec, size_t bytes_transferred)
    {
        if (!ec)
        {
            timer_.cancel();
            read_buffer_.AppendEnd(bytes_transferred);
            if (message_comming_callback_)
            {
                message_comming_callback_(shared_from_this(), read_buffer_, bytes_transferred);
            }
            SetTimeout(timeout_);
        }
        else
        {
            if (ec != asio::error::eof) {
                std::error_code ignore;
                auto remote = socket_.remote_endpoint(ignore);
                LOG_ERROR("{}:{} OnRead message:{}", remote.address().to_string(), remote.port(), ec.message());
            }
            Close();
        }
    }

    void Connection::OnWrite(const std::error_code &ec, size_t bytes_transferred)
    {
        if (!ec)
        {
            timer_.cancel();
            write_buffer_.AppendStart(bytes_transferred);
            if (write_finish_callback_)
            {
                write_finish_callback_(shared_from_this(), bytes_transferred);
            }
            SetTimeout(timeout_);
        }
        else {
            std::error_code ignore;
            auto remote = socket_.remote_endpoint(ignore);
            LOG_ERROR("{}:{} OnWrite message:{}", remote.address().to_string(), remote.port(), ec.message());
            Close();
        }
    }

    void Connection::OnTimeout(const std::error_code &ec)
    {
        if (ec == asio::error::operation_aborted) // 定时器调用 expire 重置
        {
            return;
        }
        else if (!ec) // 正常超时
        {
            if (conn_timeout_callback_)
            {
                conn_timeout_callback_(shared_from_this());
            }
        }
        else // 其他错误
        {
            std::error_code ignore;
            auto remote = socket_.remote_endpoint(ignore);
            LOG_ERROR("{}:{} OnTimeout message:{}", remote.address().to_string(), remote.port(), ec.message());
            Close();
        }
    }
    void Connection::OnHandshake(const std::error_code& ec)
    {
        return;
    }
}