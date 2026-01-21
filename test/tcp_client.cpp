#include <asio.hpp>
#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include <thread>
#include <atomic>
#include <csignal>
#include <chrono>
#include <cstring>

using asio::ip::tcp;

// 全局信号停止标志
std::atomic<bool> g_stop_flag{false};
asio::io_context io_context;

void SignalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
    {
        std::cout << "\n[Interrupt] Shutting down...\n";
        g_stop_flag = true;
        io_context.stop();
    }
}

// 异步 TCP 客户端
class TcpClient : public std::enable_shared_from_this<TcpClient>
{
public:
    TcpClient(asio::io_context &io_context,
              std::atomic<int> &success_count,
              std::atomic<int> &fail_count)
        : socket_(io_context),
          timer_(io_context),
          strand_(asio::make_strand(io_context)),
          success_count_(success_count),
          fail_count_(fail_count) {}

    void Start(const std::string &host, const std::string &port,
               const std::string &message, int timeout_seconds)
    {
        message_ = message;
        timeout_seconds_ = timeout_seconds;

        // 解析目标地址
        tcp::resolver resolver(strand_);
        auto endpoints = resolver.resolve(host, port);

        // 启动超时计时器
        timer_.expires_after(std::chrono::seconds(timeout_seconds));
        timer_.async_wait(asio::bind_executor(strand_,
                                              [self = shared_from_this()](std::error_code ec)
                                              {
                                                  if (!ec)
                                                      self->OnTimeout();
                                              }));

        // 异步连接
        asio::async_connect(socket_, endpoints,
                            asio::bind_executor(strand_,
                                                [self = shared_from_this()](std::error_code ec, tcp::endpoint)
                                                {
                                                    self->OnConnect(ec);
                                                }));
    }

private:
    void OnConnect(std::error_code ec)
    {
        timer_.cancel(); // 取消超时

        if (!ec)
        {
            success_count_++;
            // std::cout << "? Connected [" << socket_.remote_endpoint()
            //           << "] (Success: " << success_count_.load() << ")\n";

            if (!message_.empty())
            {
                // 发送消息
                asio::async_write(socket_, asio::buffer(message_),
                                  asio::bind_executor(strand_,
                                                      [self = shared_from_this()](std::error_code ec, size_t)
                                                      {
                                                          self->OnWrite(ec);
                                                      }));
            }
            else
            {
                DoRead();
            }
        }
        else
        {
            fail_count_++;
            std::cerr << "? Connect failed: " << ec.message()
                      << " (Failed: " << fail_count_.load() << ")\n";
        }
    }

    void OnWrite(std::error_code ec)
    {
        if (!ec)
        {
            std::cout << "  Message sent (" << message_.size() << " bytes)\n";
            DoRead();
        }
        else
        {
            std::cerr << "  Send failed: " << ec.message() << "\n";
        }
    }

    void DoRead()
    {
        socket_.async_read_some(asio::buffer(buffer_),
                                asio::bind_executor(strand_,
                                                    [self = shared_from_this()](std::error_code ec, size_t length)
                                                    {
                                                        self->OnRead(ec, length);
                                                    }));
    }

    void OnRead(std::error_code ec, size_t length)
    {
        if (!ec)
        {
            std::cout << "  Received: " << std::string(buffer_.data(), length) << "\n";
            DoRead();
        }
    }

    void OnTimeout()
    {
        if (!socket_.is_open())
            return;
        fail_count_++;
        std::cerr << "? Timeout (Failed: " << fail_count_.load() << ")\n";
        socket_.close();
    }

    tcp::socket socket_;
    asio::steady_timer timer_;
    asio::any_io_executor strand_;
    std::string message_;
    int timeout_seconds_;
    std::array<char, 1024> buffer_;
    std::atomic<int> &success_count_;
    std::atomic<int> &fail_count_;
};

// 连接管理器
class ConnectionManager
{
public:
    ConnectionManager(asio::io_context &io_context, int thread_count)
        : io_context_(io_context),
          work_guard_(io_context.get_executor())
    {

        // 创建工作线程池
        for (int i = 0; i < thread_count; ++i)
        {
            threads_.emplace_back([this]()
                                  { io_context_.run(); });
        }
    }

    ~ConnectionManager()
    {
        work_guard_.reset(); // 允许 io_context 停止
        for (auto &t : threads_)
        {
            if (t.joinable())
                t.join();
        }
    }

    void CreateConnections(const std::string &host, const std::string &port,
                           int connection_count, const std::string &message,
                           int timeout_seconds, int delay_ms)
    {
        for (int i = 0; i < connection_count && !g_stop_flag; ++i)
        {
            auto client = std::make_shared<TcpClient>(io_context_, success_count_, fail_count_);
            client->Start(host, port, message, timeout_seconds);

            // 控制连接速率，避免 SYN Flood
            if (delay_ms > 0 && i % 100 == 0)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            }
        }
    }

    void PrintStats() const
    {
        std::cout << "\n═══════════════════════════════════════\n"
                  << "  Connection Statistics\n"
                  << "═══════════════════════════════════════\n"
                  << "  Successful: " << success_count_.load() << "\n"
                  << "  Failed:     " << fail_count_.load() << "\n"
                  << "  Total:      " << (success_count_.load() + fail_count_.load()) << "\n"
                  << "═══════════════════════════════════════\n";
    }

private:
    asio::io_context &io_context_;
    asio::executor_work_guard<asio::io_context::executor_type> work_guard_;
    std::vector<std::thread> threads_;
    std::atomic<int> success_count_{0};
    std::atomic<int> fail_count_{0};
};

// 打印帮助
void PrintUsage(const char *program_name)
{
    std::cerr << "Usage: " << program_name << " <host> <port> [options]\n\n"
              << "Options:\n"
              << "  -c, --connections <n>  Number of connections to create (default: 100)\n"
              << "  -t, --threads <n>      Worker threads (default: CPU cores)\n"
              << "  -m, --message <text>   Message to send after connect\n"
              << "  --timeout <seconds>    Connection timeout (default: 10)\n"
              << "  --delay <ms>           Delay per 100 connections (default: 0)\n"
              << "  -h, --help             Show this help\n\n"
              << "Examples:\n"
              << "  " << program_name << " 127.0.0.1 8080\n"
              << "  " << program_name << " 192.168.1.100 9999 -c 5000 -t 8 -m \"ping\"\n"
              << "  " << program_name << " example.com 80 -c 10000 --delay 50\n";
}

int main(int argc, char *argv[])
{
    // 默认参数
    std::string host, port;
    int connection_count = 100;
    int thread_count = std::thread::hardware_concurrency();
    std::string message;
    int timeout_seconds = 10;
    int delay_ms = 0;

    // 解析命令行
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help")
        {
            PrintUsage(argv[0]);
            return 0;
        }
        else if (i == 1)
        {
            host = arg;
        }
        else if (i == 2)
        {
            port = arg;
        }
        else if ((arg == "-c" || arg == "--connections") && ++i < argc)
        {
            connection_count = std::stoi(argv[i]);
        }
        else if ((arg == "-t" || arg == "--threads") && ++i < argc)
        {
            thread_count = std::stoi(argv[i]);
        }
        else if ((arg == "-m" || arg == "--message") && ++i < argc)
        {
            message = argv[i];
        }
        else if (arg == "--timeout" && ++i < argc)
        {
            timeout_seconds = std::stoi(argv[i]);
        }
        else if (arg == "--delay" && ++i < argc)
        {
            delay_ms = std::stoi(argv[i]);
        }
    }

    if (host.empty() || port.empty())
    {
        PrintUsage(argv[0]);
        return 1;
    }

    // 注册信号
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

#ifdef _WIN32
    // // Windows 需要 WSAStartup
    // WSADATA wsa_data;
    // WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

    ConnectionManager manager(io_context, thread_count);
    try
    {

        std::cout << "╔═══════════════════════════════════════╗\n"
                  << "║      TCP Client Connection Tool       ║\n"
                  << "╚═══════════════════════════════════════╝\n\n"
                  << "Target:   " << host << ":" << port << "\n"
                  << "Count:    " << connection_count << "\n"
                  << "Threads:  " << thread_count << "\n"
                  << "Timeout:  " << timeout_seconds << "s\n"
                  << "Message:  " << (message.empty() ? "(none)" : message) << "\n"
                  << "Delay:    " << delay_ms << "ms/100conn\n"
                  << "\nPress Ctrl+C to stop\n\n";

        manager.CreateConnections(host, port, connection_count, message, timeout_seconds, delay_ms);

        // 运行事件循环直到完成或中断
        io_context.run();
        while (!g_stop_flag)
            ;
        // {
        //     // 继续处理事件
        // }
        manager.PrintStats();
    }
    catch (const std::exception &e)
    {
        std::cerr << "\nFatal error: " << e.what() << "\n";
        return 1;
    }

#ifdef _WIN32
    // WSACleanup();
#endif

    return 0;
}