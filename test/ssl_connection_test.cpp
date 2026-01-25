#include <server.h>
#include <logger.h>
#include <connection_template.hpp>
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
            //jl::ssl::stream<jl::net::socket> stream(std::move(socket), jl::GetSslContext());
            auto ssl_connction = std::make_shared<jl::SSLConnection>(tcp_server_.GetIoContext(), std::move(socket));
            ssl_connction->SetConnTimeoutCallback([](const std::shared_ptr<jl::BaseConnection>& conn){
                conn->Close();
            });
            ssl_connction->SetHandshakeCallback([](const std::shared_ptr<jl::BaseConnection>& conn){
                conn->Read();
            });
            ssl_connction->SetMessageCommingCallback([](const std::shared_ptr<jl::BaseConnection>& conn, jl::ConstBuffer& buffer) {
				std::string data(static_cast<const char*>(buffer.data()), buffer.size());
				LOG_DEBUG("MessageCommingCallback: {}", data);
				conn->Write(&data[0], data.size());
				});
            ssl_connction->SetWriteFinishCallback([](const std::shared_ptr<jl::BaseConnection>& conn,std::size_t read_bytes){
                conn->Read();
            });
            ssl_connction->SetConnCloseCallback([](const std::shared_ptr<jl::BaseConnection>& conn){
                LOG_INFO("conn close");
            });
            ssl_connction->Start();
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
