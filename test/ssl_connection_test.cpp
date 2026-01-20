#include <server.h>
#include <logger.h>
#include <ssl_connction.h>
#include <global.h>

class SSLServer
{
public:
    SSLServer(asio::io_context &ioct, const std::string &ip, unsigned short port) : tcp_server_(ioct, ip, port) {   
        tcp_server_.DoAwaitStop();
        tcp_server_.SetConnEstablishCallback([](const std::shared_ptr<jl::Connection>& conn){
            // conn->SetTimeout(2);
            auto ssl_connction = std::make_shared<jl::SslConnection>(conn, jl::GetSslContext());
            ssl_connction->SetConnTimeoutCallback([](const std::shared_ptr<jl::SslConnection>& conn){
                conn->Close();
            });
            ssl_connction->Start();
            ssl_connction->SetHandshakeCallback([](const std::shared_ptr<jl::SslConnection>& conn){
                conn->Read();
            });
            ssl_connction->SetMessageCommingCallback([](const std::shared_ptr<jl::SslConnection>& conn,std::size_t read_bytes, jl::Buffer& buffer){
                std::string data = buffer.ReadAsString(buffer.Size());
                conn->Write(data);
            });
            ssl_connction->SetWriteFinishCallback([](const std::shared_ptr<jl::SslConnection>& conn,std::size_t read_bytes){
                conn->Read();
            });
            ssl_connction->SetConnCloseCallback([](const std::shared_ptr<jl::SslConnection>& conn){
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
    return 0;
}
