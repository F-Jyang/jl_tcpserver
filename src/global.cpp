#include "global.h"

#include <define.h>

namespace jl {

#ifdef ENABLE_OPENSSL

#ifdef ENABLE_SSL_V2
	asio::ssl::context gSslContext{ ssl::context::tlsv12_server };
#else
	asio::ssl::context gSslContext{ ssl::context::tlsv13_server };
#endif
	asio::ssl::context& jl::GetSslContext()
	{
		return gSslContext;
	}
 
	// TODO:设置gSslContext的证书、私钥、CA等


#endif // ENABLE_OPENSSL

}

