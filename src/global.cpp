#include "global.h"
#include <logger.h>
#include <define.h>

namespace jl {

#ifdef ENABLE_OPENSSL
	asio::ssl::context gSslContext{ ssl::context::sslv23 }; // 自动协商TLS版本

	bool InitSslContext() {
		try {
			gSslContext.set_options(
				ssl::context::default_workarounds |  // 兼容性补丁
				ssl::context::no_sslv2 | // 禁用不安全的sslV2
				ssl::context::single_dh_use // 启动DH密钥单次使用
			);

			// 设置私钥密码回调
			gSslContext.set_password_callback([](std::size_t, ssl::context::password_purpose) {return ""; });

			// 加载服务器证书链文件（含证书和中间CA）
			gSslContext.use_certificate_chain_file("xxx.pem");

			// 加载私钥文件（pem格式，通常跟证书在同一文件）
			gSslContext.use_private_key_file("xxx.pem", ssl::context::pem);

			// 加载临时DH参数文件（用于DHE密钥交换），每次生成新DH密钥，防止密钥重复攻击，实现完美前向保护（PFS）
			gSslContext.use_tmp_dh_file("xxx.pem");
		}
		catch (const std::exception& err) {
			LOG_ERROR("fail: {}!", err.what());
			return false;
		}
		return true;
	}

	asio::ssl::context& jl::GetSslContext()
	{
		return gSslContext;
	}

#endif // ENABLE_OPENSSL

}

