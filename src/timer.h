#pragma once

#include <connection_template.hpp>

namespace jl {
	class Timer {
	public:
		Timer(const std::shared_ptr<BaseConnection>& conn);

		void Wait(std::size_t milli_secs);

		void SetCallback(const TimeoutCallback& callback) { callback_ = callback; }

		void Cancel();

	private:
		asio::steady_timer timer_;
		TimeoutCallback callback_;
	};
}