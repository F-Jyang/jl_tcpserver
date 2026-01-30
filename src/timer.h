/// @file timer.h
/// @brief 定时器类
/// @author Jyang.
/// @date 2026-1-30
/// @version 1.0

#pragma once

#include <connection.h>

namespace jl {
	class Timer {
	public:

		/// @brief 创建与Connection关联的定时器，要求Connection中的socket绑定asio::strand，否则多线程下可能存在并发安全问题
		/// @param conn 关联的Connection
		Timer(const std::shared_ptr<Connection>& conn);

		/// @brief 使用executor创建定时器
		/// @param executor 关联的executor
		Timer(const asio::any_io_executor& executor);

		/// @brief 异步等待 milli_secs(ms)后触发回调函数
		/// @param milli_secs 等待时长(ms)
		void Wait(std::size_t milli_secs);

		void SetCallback(const TimeoutCallback& callback) { callback_ = callback; }

		void Cancel();

	private:
		asio::steady_timer timer_;
		TimeoutCallback callback_;
	};
}