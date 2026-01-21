#include <server.h>
#include <logger.h>

class EchoServer
{
public:
    EchoServer(asio::io_context &ioct, const std::string &ip, unsigned short port) : tcp_server_(ioct, ip, port) {
        tcp_server_.DoAwaitStop();
        tcp_server_.SetConnEstablishCallback([=](jl::Socket&& socket){
            auto conn = std::make_shared<jl::Connection>(tcp_server_.GetIoContext(), std::move(socket));
            //conn->SetTimeout(3);
            conn->SetConnTimeoutCallback([](const std::shared_ptr<jl::BaseConnection>& conn) {
                LOG_DEBUG("TimeoutCallback");
                conn->Close();
                });
            conn->SetMessageCommingCallback([](const std::shared_ptr<jl::BaseConnection>& conn, jl::Buffer& buffer, std::size_t read_bytes) {
                std::string data = buffer.ReadAll();
                LOG_DEBUG("MessageCommingCallback: {}", data);
                conn->Write(data);
                });
            conn->SetWriteFinishCallback([](const std::shared_ptr<jl::BaseConnection>& conn, std::size_t read_bytes) {
                LOG_DEBUG("WriteFinishCallback");
                conn->Read();
                });
            conn->SetConnCloseCallback([](const std::shared_ptr<jl::BaseConnection>& conn) {
                LOG_DEBUG("CloseCallback");
                });
            conn->Start();

        });

    }

    void Start() { tcp_server_.Start(); }

private:
    jl::Server tcp_server_;
};

int main(int argc, char const *argv[])
{
    asio::io_context ioct;
    EchoServer server(ioct, "127.0.0.1", 12345);
    server.Start();
    return 0;
}
