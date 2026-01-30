#include <http_server.h>

int main()
{
    std::shared_ptr<HttpServer> server = std::make_shared<HttpServer>("127.0.0.1", 9999);
    server->Start();
}