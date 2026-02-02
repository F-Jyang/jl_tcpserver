#pragma once
#include <asio/ssl.hpp>
#include <define.h>

namespace jl {

	struct Global {
	public:
		static Global& Instance();

#ifdef ENABLE_OPENSSL
		bool InitSSLContext();

		asio::ssl::context& GetSSLContext();

		void SetCRTFilePath(const std::string& filename);

		void SetPrivateKeyPath(const std::string& filename);

		void SetTmpDhPath(const std::string& filename);

		void SetPasswordCallback(const std::function<std::string(std::size_t, ssl::context::password_purpose)>& callback);
	private:
		asio::ssl::context ssl_context_;
		std::string crt_file_;
		std::string private_key_file_;
		std::string tmp_db_file_;
		std::function<std::string(std::size_t, ssl::context::password_purpose)> password_callback_;
#endif

	private:
		Global();
		Global(const Global&) = delete;
		Global& operator=(const Global&) = delete;
	};
}