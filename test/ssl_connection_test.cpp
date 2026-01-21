#include <server.h>
#include <logger.h>
#include <ssl_connction.h>
#include <global.h>

std::atomic<int> gConnCnt = 0;

class SSLServer
{
public:
    SSLServer(asio::io_context &ioct, const std::string &ip, unsigned short port) : tcp_server_(ioct, ip, port) {   
        tcp_server_.DoAwaitStop();
        tcp_server_.SetConnEstablishCallback([=](jl::Socket&& socket) {
            // conn->SetTimeout(2);
            gConnCnt.fetch_add(1,std::memory_order_relaxed);
            auto ssl_connction = std::make_shared<jl::SslConnection>(tcp_server_.GetIoContext(), jl::GetSslContext(), std::move(socket));
            ssl_connction->SetConnTimeoutCallback([](const std::shared_ptr<jl::BaseConnection>& conn){
                conn->Close();
            });
            ssl_connction->Start();
            ssl_connction->SetHandshakeCallback([](const std::shared_ptr<jl::BaseConnection>& conn){
                conn->Read();
            });
            ssl_connction->SetMessageCommingCallback([](const std::shared_ptr<jl::BaseConnection>& conn, jl::Buffer& buffer, std::size_t read_bytes){
                std::string data = buffer.ReadAsString(buffer.Size());
                conn->Write(data);
            });
            ssl_connction->SetWriteFinishCallback([](const std::shared_ptr<jl::BaseConnection>& conn,std::size_t read_bytes){
                conn->Read();
            });
            ssl_connction->SetConnCloseCallback([](const std::shared_ptr<jl::BaseConnection>& conn){
                LOG_INFO("conn close");
            });
        });

    }

    void Start() { tcp_server_.Start(); }

private:
    jl::Server tcp_server_;
};

int main(int argc, char const *argv[])
{
    asio::io_context ioct;
    SSLServer server(ioct, "127.0.0.1", 12345);
    server.Start();
    LOG_INFO("Connection count: {}", gConnCnt.load());
    return 0;
}
