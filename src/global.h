#pragma once
#include <asio/ssl.hpp>

namespace jl {
#ifdef ENABLE_OPENSSL
	asio::ssl::context& GetSslContext();
#endif
}