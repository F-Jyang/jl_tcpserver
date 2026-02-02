#include "global.h"
#include <logger.h>

namespace jl {

	Global::Global() :
#ifdef ENABLE_OPENSSL
		ssl_context_(ssl::context::tlsv12_server)
#endif
	{


	}


#ifdef ENABLE_OPENSSL
	bool Global::InitSSLContext()
	{
		try {
			ssl_context_.set_options(
				ssl::context::default_workarounds |  // 兼容性补丁
				ssl::context::no_sslv2 | // 禁用不安全的sslV2
				ssl::context::no_sslv3 | // 禁用不安全的sslV2
				ssl::context::single_dh_use // 启动DH密钥单次使用
			);

			// 设置私钥密码回调
			ssl_context_.set_password_callback(password_callback_);

			// 加载服务器证书链文件（含证书和中间CA）
			ssl_context_.use_certificate_chain_file(crt_file_);

			// 加载私钥文件（pem格式，通常跟证书在同一文件）
			ssl_context_.use_private_key_file(private_key_file_, ssl::context::pem);

			// 加载临时DH参数文件（用于DHE密钥交换），每次生成新DH密钥，防止密钥重复攻击，实现完美前向保护（PFS）
			ssl_context_.use_tmp_dh_file(tmp_db_file_);
		}
		catch (const std::exception& err) {
			LOG_ERROR("fail: {}!", err.what());
			return false;
		}
		return true;
	}

	asio::ssl::context& Global::GetSSLContext()
	{
		return ssl_context_;
	}

	void Global::SetCRTFilePath(const std::string& filename)
	{
		crt_file_ = filename;
	}

	void Global::SetPrivateKeyPath(const std::string& filename)
	{
		private_key_file_ = filename;
	}

	void Global::SetTmpDhPath(const std::string& filename)
	{
		tmp_db_file_ = filename;
	}
	void Global::SetPasswordCallback(const std::function<std::string(std::size_t, ssl::context::password_purpose)>& callback)
	{
		password_callback_ = callback;
	}
#endif // ENABLE_OPENSSL

	Global& Global::Instance()
	{
		static Global instance;
		return instance;
	}
}

