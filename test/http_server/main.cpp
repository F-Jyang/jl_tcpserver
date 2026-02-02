#include <http_server.h>
#include <global.h>

int main()
{
	jl::Global::Instance().SetCRTFilePath("./resource/server.crt");
	jl::Global::Instance().SetPrivateKeyPath("./resource/server.key");
	jl::Global::Instance().SetTmpDhPath("./resource/dh2048.pem");
	jl::Global::Instance().SetPasswordCallback([](std::size_t, jl::ssl::context::password_purpose) {return ""; });
	jl::Global::Instance().SetCRTFilePath("./resource/server.crt");
    jl::Global::Instance().InitSSLContext();

    std::shared_ptr<HttpServer> server = std::make_shared<HttpServer>("127.0.0.1", 9999);
    server->Start();
}