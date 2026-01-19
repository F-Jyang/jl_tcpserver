#include <server.h>

class EchoServer
{
public:
    EchoServer(asio::io_context &ioct, const std::string &ip, unsigned short port) : tcp_server_(ioct, ip, port) {
        tcp_server_.DoAwaitStop();
        tcp_server_.SetConnEstablishCallback([](const std::shared_ptr<jl::Connection>& conn){
            // conn->SetTimeout(2);
            conn->SetConnTimeoutCallback([](const std::shared_ptr<jl::Connection>& conn){
                conn->Close();
            });
            conn->SetMessageCommingCallback([](const std::shared_ptr<jl::Connection>& conn,std::size_t read_bytes, jl::Buffer& buffer){
                std::string data = buffer.ReadAsString(buffer.Size());
                conn->Write(data);
            });
            conn->SetWriteFinishCallback([](const std::shared_ptr<jl::Connection>& conn,std::size_t read_bytes){
                conn->Read();
            });
            conn->Read();

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
