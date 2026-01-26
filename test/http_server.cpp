#include <server.h>
#include <connection_template.hpp>
#include <logger.h>
#include <map>

class HttpServer
{
public:
    HttpServer(const std::string &ip, unsigned short port) : tcp_server_(ioct_, ip, port)
    {
        tcp_server_.SetConnEstablishCallback([=](jl::Socket &&socket)
                                             {
            std::shared_ptr<jl::Connection> conn = std::make_shared<jl::Connection>(ioct_, std::move(socket));
            //conn_map_.emplace(conn.get(), conn);
            LOG_INFO("New connection comming.");
        
			//conn->SetMessageCommingCallback([=](const std::shared_ptr<jl::BaseConnection> &conn, jl::ConstBuffer &buffer) {
			conn->SetMessageCommingCallback([=](const std::shared_ptr<jl::BaseConnection> &conn, const std::string &buffer) {
                std::string str(static_cast<const char*>(buffer.data()), buffer.size());
                LOG_INFO("Read data: {}", str);
                std::string response = "<b>" + str + "</b>";
                conn->ReadUntil("\r\n");
            });
            
            conn->SetWriteFinishCallback([=](const std::shared_ptr<jl::BaseConnection> &conn, std::size_t bytes_transferred) {
                LOG_INFO("Write finish: {}", bytes_transferred);
            });

            conn->SetConnCloseCallback([=](const std::shared_ptr<jl::BaseConnection> &conn) {
				//conn_map_.erase(conn.get());
                LOG_INFO("Connection closed.");
            });

            conn->SetConnTimeoutCallback([=](const std::shared_ptr<jl::BaseConnection> &conn) {
                LOG_INFO("Connection Timeout.");    
            });
            conn->ReadUntil("\r\n"); });
    }

    void Start()
    {
        tcp_server_.Start();
    }

    void Stop()
    {
        tcp_server_.Stop();
    }

private:
    //std::map<jl::BaseConnection*, std::shared_ptr<jl::BaseConnection>> conn_map_;
    asio::io_context ioct_;
    jl::Server tcp_server_;
};

int main()
{
    HttpServer server("127.0.0.1", 12345);
    server.Start();
}