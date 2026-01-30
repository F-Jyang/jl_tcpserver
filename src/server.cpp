#include "server.h"
#include <logger.h>

jl::Server::Server(asio::io_context& ioct, const std::string& ip, unsigned short port) :
    ioct_(ioct),
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
                try
                {
                    ioct_.run();
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("error: {}", e.what());
                }
            }));
        --thread_cnt;
    }
    while(!stop_) { // 等待停止
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

void jl::Server::Stop()
{
    LOG_WARN("Server stop.");
    ioct_.stop();
    for (std::size_t i = 0; i < io_threads_.size(); ++i)
    {
        if (io_threads_[i] && io_threads_[i]->joinable())
        {
            io_threads_[i]->join();
        }
    }
    io_threads_.clear();
    LOG_WARN("Server stop finish.");
}

asio::io_context& jl::Server::GetIoContext()
{
    return ioct_;
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
    signals_.add(SIGTERM);
    WaitSignal();
}

void jl::Server::WaitSignal()
{
    signals_.async_wait(
        [=](std::error_code ec, int sig)
        {
            if (ec)
                return;
            LOG_WARN("Caught signal:{}", sig);
            stop_ = true;
            // this->Stop(); // bug: 这里调用Stop，join的时候可能会在非主线程调用，导致崩溃
            // WaitSignal();  // 如果想继续捕获，再发起一次等待
        });
}
