#include <server.h>
#include <logger.h>
#include <timer.h>
#include <socket.h>

class EchoServer
{
public:
    EchoServer(asio::io_context &ioct, const std::string &ip, unsigned short port) : tcp_server_(ioct, ip, port) {
        tcp_server_.DoAwaitStop();
        tcp_server_.SetConnEstablishCallback([=](jl::net::socket&& socket){
            auto conn = jl::MakeConnection(std::move(socket));
            auto timer = std::make_shared<jl::Timer>(conn);
            timer->SetCallback([conn]() {
                LOG_ERROR("TimeoutCallback");
                conn->Close();
                });

			conn->SetMessageCommingCallback([=](const std::shared_ptr<jl::Connection>& conn, const std::string& buffer) {
                timer->Cancel();
                std::string data(static_cast<const char*>(buffer.data()),buffer.size());
                LOG_DEBUG("MessageCommingCallback: {}", data);
                conn->Write(data+"1");
                conn->Write(data+"2");
                conn->Write(data+"3");
                timer->Wait(10000);
                });
            conn->SetWriteFinishCallback([=](const std::shared_ptr<jl::Connection>& conn, std::size_t read_bytes) {
                timer->Cancel();
                LOG_DEBUG("WriteFinishCallback");
                conn->Read();
                timer->Wait(10000);
                });
            conn->SetConnCloseCallback([=](const std::shared_ptr<jl::Connection>& conn) {
                LOG_DEBUG("CloseCallback");
                });
            //conn->ReadUntil("world");
            timer->Wait(10000);
            conn->Read();

        });

    }

    void Start() { tcp_server_.Start(); }

private:
    jl::Server tcp_server_;
};

//void data(const void* data, std::size_t size) {
//    asio::streambuf buf;
//    std::ostream os(&buf);
//    os.write(static_cast<const char*>(data), size);
//    
//    asio::const_buffer input = buf.data();
//    std::string s(static_cast<const char*>(input.data()), input.size());
//    std::cout << s << std::endl;
//}

int main(int argc, char const *argv[])
{
    asio::io_context ioct;
    EchoServer server(ioct, "127.0.0.1", 12345);
    //data("hello world", 11);
    server.Start();
    return 0;
}
