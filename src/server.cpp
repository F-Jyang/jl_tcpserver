#include "server.h"

jl::Server::Server(asio::io_context &ioct, const std::string &ip, unsigned short port) : ioct_(ioct),
                                                                                         acceptor_(std::make_shared<Acceptor>(ioct, ip, port)),
                                                                                         signals_(ioct),
                                                                                         stop_(true)
{
}

void jl::Server::Start(std::size_t thread_cnt)
{
    stop_ = false;
    acceptor_->DoAccept();
    // DoAwaitStop();
    while (thread_cnt > 0)
    {
        io_threads_.emplace_back(std::make_unique<std::thread>(
            [=]()
            {
                ioct_.run();
            }));
        --thread_cnt;
    }
    ioct_.run();
}

void jl::Server::Stop()
{
    bool expect = false;
    this->acceptor_;
    if (!stop_.compare_exchange_strong(expect, true))
    { // 避免二次Stop导致对io_threads_的重复join（二次join似乎会导致unique_ptr<thread>的崩溃）
        return;
    }
    std::cout << "stop" << std::endl;
    ioct_.stop();
    std::cout << io_threads_.size() << std::endl;
    for (std::size_t i = 0; i < io_threads_.size(); ++i)
    {
        std::cout << i << std::endl;
        if (io_threads_[i] && io_threads_[i]->joinable())
        {
            io_threads_[i]->join();
        }
    }
    io_threads_.clear();
    std::cout << "stop finish" << std::endl;
}

void jl::Server::SetConnEstablishCallback(const ConnEstablishCallback &callback)
{
    acceptor_->SetConnEstablishCallback(callback);
}

jl::Server::~Server()
{
    Stop();
}

void jl::Server::DoAwaitStop()
{
    signals_.add(SIGINT);
    WaitSignal();
}

void jl::Server::WaitSignal()
{
    signals_.async_wait(
        [=](std::error_code ec, int sig)
        {
            if (ec)
                return;
            std::cout << "caught " << sig << ", shutting down…\n";
            ioct_.stop();
            // this->Stop(); // bug: 这里调用Stop，join的时候可能会在非主线程调用，导致崩溃
            // WaitSignal();  // 如果想继续捕获，再发起一次等待
        });
}
