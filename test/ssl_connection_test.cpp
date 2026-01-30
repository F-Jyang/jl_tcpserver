#include <server.h>
#include <logger.h>
#include <connection.h>
#include <global.h>

std::atomic<int> gConnCnt = 0;

class SSLServer
{
public:
    SSLServer(asio::io_context &ioct, const std::string &ip, unsigned short port) : tcp_server_(ioct, ip, port) {   
        tcp_server_.DoAwaitStop();
        tcp_server_.SetConnEstablishCallback([=](jl::net::socket&& socket) {
            // conn->SetTimeout(2);
            gConnCnt.fetch_add(1,std::memory_order_relaxed);
            auto ssl_connction = jl::MakeSSLConnection(std::move(socket));
            //ssl_connction->SetConnTimeoutCallback([](const std::shared_ptr<jl::BaseConnection>& conn){
            //    conn->Close();
            //});
            ssl_connction->SetHandshakeCallback([](const std::shared_ptr<jl::Connection>& conn){
                conn->Read();
            });
            ssl_connction->SetMessageCommingCallback([](const std::shared_ptr<jl::Connection>& conn, const std::string& buffer) {
				std::string data(static_cast<const char*>(buffer.data()), buffer.size());
				LOG_DEBUG("MessageCommingCallback: {}", data);
				conn->Write(&data[0], data.size());
				});
            ssl_connction->SetWriteFinishCallback([](const std::shared_ptr<jl::Connection>& conn,std::size_t read_bytes){
                conn->Read();
            });
            ssl_connction->SetConnCloseCallback([](const std::shared_ptr<jl::Connection>& conn){
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
