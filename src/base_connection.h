/// @brief 基础连接类，定义了连接的基本行为
/// @note 原本想使用双strand分别封装read、write，都是发现很复杂，
///       使用asio::bind_executor传入的strand可能会跟socket本身的executor冲突，导致回调不会执行的bug
//        同时双strand的性能似乎也没有很好，最后还是使用单strand  



#pragma once
#include <define.h>
#include <queue>

namespace jl
{
    constexpr std::size_t kDefaultMaxReadBytes = 2048;
    constexpr std::size_t kDefaultTimeout = 5 * 60;
    constexpr std::size_t kDefaultBufferMaxSize = 1024 * 4;

    enum class ConnectionState {
        kActived = 1,
        kClosing,
        kClosed,
    };

    enum class Option {
        RD_ONLY = 1,
        WR_ONLY = 1 << 1,
        RDWR = RD_ONLY + WR_ONLY,
    };
    
    inline bool operator&(Option opt1, Option opt2) {
        return static_cast<int>(opt1) & static_cast<int>(opt2);
    }

    inline Option operator|(Option opt1, Option opt2) {
        return static_cast<Option>(static_cast<int>(opt1) | static_cast<int>(opt2));
    }

    class BaseConnection : public std::enable_shared_from_this<BaseConnection>
    {
    public:
        BaseConnection(asio::io_context& ioct, std::size_t buffer_max_size = kDefaultBufferMaxSize);
        
        /// @brief 启动连接，开始异步读取数据
        virtual void Start() = 0;
       
        /// @brief 异步读取数据
        /// @param max_bytes 最大读取字节数，默认值为 kDefaultMaxReadBytes
        virtual void Read() = 0;

        virtual void ReadN(std::size_t exactly_bytes) = 0;

        virtual void ReadUntil(const std::string& sep) = 0;

        virtual const asio::any_io_executor& GetExecutor() = 0;

		//virtual void ReadWithTimeout(std::size_t secs, std::size_t max_bytes = kDefaultMaxReadBytes) = 0;

		//virtual void ReadNWithTimeout(std::size_t secs, std::size_t exactly_bytes) = 0;

		//virtual void ReadUntilWithTimeout(std::size_t secs, const std::string& sep, std::size_t max_bytes = kDefaultMaxReadBytes) = 0;

        /// @brief 异步写入数据
        /// @param data 数据指针
        /// @param n 数据字节数
		virtual void Write(const void* data, std::size_t n) = 0;
		
        //virtual void WriteWithTimeout(std::size_t secs, const void* data, std::size_t n) = 0;

        /// @brief 异步写入数据
        /// @param data 数据字符串
        virtual void Write(const std::string& data) = 0;

        virtual void DoWrite() = 0;
         
        /// @brief 关闭连接
        virtual void Close() = 0;

        virtual net::endpoint GetRemoteEndpoint() const = 0;

        virtual net::endpoint GetLocalEndpoint() const = 0;

        ///// @brief 设置定时器超时时间
        ///// @param secs 超时时间，单位为秒，默认值为 kDefaultTimeout
        //void SetTimeout(std::size_t secs = kDefaultTimeout);

        /// @brief 设置写入完成回调函数
        /// @param callback 写入完成回调函数
        void SetWriteFinishCallback(WriteFinishCallback callback) { write_finish_callback_ = callback;}

        /// @brief 设置写入完成回调函数
        /// @param callback 写入完成回调函数
        void SetHandshakeCallback(HandshakeCallback callback) { handshake_callback_ = callback; }

        /// @brief 设置消息到达回调函数
        /// @param callback 消息到达回调函数
        void SetMessageCommingCallback(MessageCommingCallback callback) { message_comming_callback_ = callback; }

        /// @brief 设置连接关闭回调函数
        /// @param callback 连接关闭回调函数
        void SetConnCloseCallback(ConnCloseCallback callback) { conn_close_callback_ = callback; }

        ///// @brief 设置连接超时回调函数
        ///// @param callback 连接超时回调函数
        //void SetConnTimeoutCallback(ConnTimeoutCallback callback) { conn_timeout_callback_ = callback; }

        asio::io_context &GetIoContext();

        virtual ~BaseConnection() {}

    //protected:

        /// @brief 处理读取完成事件
        /// @param ec 错误码
        /// @param bytes_transferred 实际读取字节数
        virtual void OnRead(const std::error_code &ec, size_t bytes_transferred, std::size_t sep_len) = 0;

        /// @brief 处理写入完成事件
        /// @param ec 错误码
        /// @param bytes_transferred 实际写入字节数
        virtual void OnWrite(const std::error_code &ec, size_t bytes_transferred) = 0;

        ///// @brief 处理超时事件
        ///// @param ec 错误码
        //virtual void OnTimeout(const std::error_code &ec) = 0;

        /// @brief 握手完成事件
        /// @param ec 错误码
        virtual void OnHandshake(const std::error_code &ec) = 0;
    
        /// @brief 异步执行任务，确保在 strand 中执行
        /// @tparam Function 可调用对象类型
        /// @tparam Allocator 分配器类型
        /// @param f 可调用对象
        /// @param a 分配器实例
        template <typename Function, typename Allocator = std::allocator<void>>
        void PostTask(Function&& f, const Allocator& a = std::allocator<void>{}) const;
   
    protected:
        // std::uint64_t id_;
        std::atomic<bool> read_in_progress_;
        std::atomic<ConnectionState> state_;
        asio::io_context &ioct_;
        //asio::strand<asio::io_context::executor_type> io_strand_;  // warning: asio::io_context::strand 不推荐使用，其部分成员函数已经被废弃。推荐使用asio::strand<>
        //asio::steady_timer timer_;
        asio::streambuf read_buffer_;
        std::queue<std::string> send_queue_;
        HandshakeCallback handshake_callback_;
        WriteFinishCallback write_finish_callback_;
        MessageCommingCallback message_comming_callback_;
        ConnCloseCallback conn_close_callback_;
        //ConnTimeoutCallback conn_timeout_callback_;
    };

    template<typename Function, typename Allocator>
    inline void BaseConnection::PostTask(Function&& f, const Allocator& a) const
    {
        asio::post(io_strand_, std::forward<Function>(f));
    }
}