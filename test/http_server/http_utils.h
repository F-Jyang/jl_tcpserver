#pragma once

#include <logger.h>
#include <string>
#include <unordered_map>
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>

struct HttpRequest{
    std::string request_line_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};

struct HttpResponse {
    std::string response_line_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;
};

inline std::string GetCurrentTimeStr() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    // 获取毫秒部分
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch() % 1000
    ).count();

    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms;
    return ss.str();
}

inline std::string GetHttpResponse(const HttpRequest& request) {
    std::stringstream request_stream;
    request_stream << request.request_line_ << "\r\n<br>";
    for (auto& it = request.headers_.begin(); it != request.headers_.end(); ++it) {
        request_stream << it->first << ": " << it->second << "\r\n<br>";
    }
    request_stream << "\r\n<br>";
    request_stream << request.body_;
    std::string request_string = fmt::format("<html><body>{}</body></html>", request_stream.str());
    std::stringstream response_stream;
    response_stream << "HTTP/1.1 200 OK\r\n";
    response_stream << "Content-Type: text/html; charset=UTF-8" << "\r\n";
    response_stream << "Date: " << GetCurrentTimeStr() << "\r\n";
    response_stream << "Content-Length: " << request_string.size() << "\r\n";
    response_stream << "\r\n";
    response_stream << request_string;
    return response_stream.str();
}