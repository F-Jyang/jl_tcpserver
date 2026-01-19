#include "connection.h"
#include <iostream>

namespace jl
{

    Connection::Connection(asio::io_context& ioct, net::socket&& socket) :
        socket_(std::move(socket)),
        io_strand_(ioct),
        timer_(ioct),
        timeout_(kDefaultTimeout),
        ioct_(ioct)
    {
    }

    void Connection::SetTimeout(std::size_t secs)
    {
        timeout_ = secs;
        auto self = shared_from_this();
        timer_.expires_after(std::chrono::seconds(secs));
        timer_.async_wait(asio::bind_executor(io_strand_, [self](const std::error_code &ec)
                                              { self->OnTimeout(ec); }));
        // PostTask([self]()
        //          { self->timer_.async_wait([self](const std::error_code &ec)
        //                                    { self->OnTimeout(ec); }); });

        // io_strand_.post(
        //     [self]()
        //     {
        //         self->timer_.async_wait([self](const std::error_code &ec)
        //                                 { self->OnTimeout(ec); });
        //     },
        //     std::allocator<void>{});
    }

    void Connection::Start()
    {
        this->Read();
    }

    net::socket &Connection::GetSocket()
    {
        return socket_;
    }

    asio::io_context& Connection::GetIoContext() {
        return ioct_;
    }

    net::socket Connection::ExtractSocket()&& {
        return std::move(socket_);
    }


    void Connection::Read(std::size_t max_bytes)
    {
        read_buffer_.EnableWrite(max_bytes);
        auto self = shared_from_this();
        PostTask([self, max_bytes]()
                 { self->socket_.async_read_some(asio::buffer(self->read_buffer_.WriteStart(), max_bytes), [=](const std::error_code &ec, size_t bytes_transferred)
                                                 { self->OnRead(ec, bytes_transferred); }); });
        // io_strand_.post(
        //     [=]()
        //     {
        //         socket_.async_read_some(asio::buffer(read_buffer_.WriteStart(), max_bytes), [=](const std::error_code &ec, size_t bytes_transferred)
        //                                 { self->OnRead(ec, bytes_transferred); });
        //     },
        //     std::allocator<void>{});
    }

    void Connection::Write(const void *data, std::size_t n)
    {
        if (n <= 0)
        {
            return;
        }
        write_buffer_.Append(data, n);
        auto self = shared_from_this();
        PostTask([self]()
                 { self->socket_.async_write_some(asio::buffer(self->write_buffer_.ReadStart(), self->write_buffer_.Size()), [=](const std::error_code &ec, size_t bytes_transferred)
                                                  { self->OnWrite(ec, bytes_transferred); }); });
        // io_strand_.post(
        //     [=]()
        //     {
        //         self->socket_.async_write_some(asio::buffer(write_buffer_.ReadStart(), write_buffer_.Size()), [=](const std::error_code &ec, size_t bytes_transferred)
        //                                        { self->OnWrite(ec, bytes_transferred); });
        //     },
        //     std::allocator<void>{});
    }

    void Connection::Write(const std::string &data)
    {
        Write(data.c_str(), data.size());
    }

    void Connection::Close()
    {
        if (socket_.is_open())
        {
            std::error_code ec;
            timer_.cancel();
            socket_.shutdown(net::socket::shutdown_both, ec);
            if (ec)
            {
                std::cerr << "shutdown: " << ec.message() << std::endl;
            }
            socket_.close(ec); // 文档要求: call shutdown() before closing the socket，否则可能会提示非法套接字，好像就算先shutdown也可能会
            if (ec)
            {
                std::cerr << "close: " << ec.message() << std::endl;
            }
            if (conn_close_callback_)
            {
                conn_close_callback_(shared_from_this());
            }
        }
    }

    Connection::~Connection()
    {
        std::cout << "Connection destructor called" << std::endl;
        Close();
    }

    void Connection::OnRead(const std::error_code &ec, size_t bytes_transferred)
    {
        if (ec)
        {
            std::cerr << ec.message() << std::endl;
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

    void Connection::OnWrite(const std::error_code &ec, size_t bytes_transferred)
    {
        if (ec)
        {
            std::cerr << ec.message() << std::endl;
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

    void Connection::OnTimeout(const std::error_code &ec)
    {
        if (ec == asio::error::operation_aborted) // 定时器调用 expire 重置
        {
            std::cout << ec.message() << std::endl;
            return;
        }
        else if (!ec) // 正常超时
        {
            std::cerr << ec.message() << std::endl;
            if (conn_timeout_callback_)
            {
                conn_timeout_callback_(shared_from_this());
            }
        }
        else // 其他错误
        {
            std::cerr << ec.message() << std::endl;
            Close();
        }
    }
}