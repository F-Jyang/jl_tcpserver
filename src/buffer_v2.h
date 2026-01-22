/// @file buffer.h
/// @brief 缓冲区类
/// @author Jyang.
/// @date 2026-1-17
/// @version 1.0

#pragma once

#include <vector>
#include <assert.h>
#include <asio/buffer.hpp>

namespace jl
{
    constexpr std::size_t kDefaultCapacity = 1024 * 2;
    constexpr std::size_t kDefaultMaxSize = 1024 * 1024;

   
    class Connection;
    class SslConnection;

    class Buffer {
        friend class Connection;
        friend class SslConnection;
    public:
        // 类型定义
        using const_buffers_type = std::vector<asio::const_buffer>;
        using mutable_buffers_type = std::vector<asio::mutable_buffer>;


        // 指定初始容量
        explicit Buffer(size_t initial_capacity = kDefaultCapacity);

        Buffer(Buffer&& other);

        Buffer(const Buffer& other);

        // 返回当前数据大小
        size_t size() const noexcept;

        // 返回最大允许大小
        size_t max_size() const noexcept;

        void set_max_size(size_t size);

        // 返回当前容量（无需重新分配即可容纳的大小）
        size_t capacity() const noexcept;

        // 返回从 pos 开始的 n 字节常量缓冲区序列
        const_buffers_type data(size_t pos, size_t n) const;

        // 返回从 pos 开始的 n 字节可变缓冲区序列
        mutable_buffers_type data(size_t pos, size_t n);

        // 在末尾扩展 n 个字节
        void grow(size_t n);

        // 从末尾移除 n 个字节
        void shrink(size_t n);

        // 从开头移除 n 个字节（需要 memmove）
        void consume(size_t n);

        // 便捷方法：获取所有数据
        const char* data() const noexcept;

        char* data() noexcept;

        // 清空
        void clear() noexcept;

        std::size_t available() const;

        /// @brief 添加一个类型T的数据到buffer
        /// @tparam T 要求为POD类型或基础数据类型
        /// @param data
        template <typename T, typename DecayT = std::decay_t<T>, typename = std::enable_if_t<std::is_fundamental_v<DecayT> || std::is_pod_v<DecayT>>>
        void Append(T&& data)
        {
            static_assert(!std::is_pointer_v<DecayT>, "Buffer::Append: T can't be pointer type."); // cpp17及17之前的bug，pointer会被识别为pod类型
            this->Append(std::addressof(data), sizeof(DecayT));
        }

        const char* read_start() const;

        char* write_start();

        template <typename T>
        std::vector<T> Read(std::size_t n)
        {
            std::vector<T> result;
            if (n <= 0)
            {
                assert(false);
                return result;
            }
            std::size_t readable_bytes = size();
            std::size_t max_num = readable_bytes / sizeof(T);
            n = std::min(max_num, n);
            result.resize(n);
            char* dest = (char*)&result[0];
            std::copy(read_start(), read_start() + n * sizeof(T), dest);
            start_ += n * sizeof(T);
            return result;
        }

        /// @brief 确保缓冲区有足够的空间写入数据
        /// @param len
        void EnableWrite(std::size_t len);
    private:

        /// @brief 扩展缓冲区容量
        /// @param len 扩展长度
        void Expand(size_t len);

        /// @brief 移动数据到缓冲区开头
        void MoveData();

        void MoveStart(size_t len) { start_ += len; }

        void MoveEnd(size_t len) { end_ += len; }
    private:
        std::size_t max_size_;
        std::vector<char> buffer_;
        size_t start_, end_;
    };

    /*
    struct Buffer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using mutable_buffers_type = asio::mutable_buffer;

        Buffer(std::size_t capacity = kDefaultCapacity);

        const_buffers_type data() const;

        void consume(std::size_t len);

        void commit(std::size_t len);

        std::size_t size() const;

        std::size_t max_size() const;

        asio::mutable_buffer prepare(std::size_t len);

        std::size_t capacity() { return buffer_.size(); };

        ~Buffer();

        /// @brief 添加包含N个基础数据类型数据的数组到beffer。注意对于类似 "hello world" 的char数组长度包含了尾部的\0
        /// @tparam T
        /// @tparam
        /// @tparam N
        /// @param data
        template <typename T, typename = std::enable_if_t<std::is_fundamental_v<std::decay_t<T>>>, size_t N>
        void Append(const T (&data)[N])
        {
            this->Append(data, N * sizeof(T));
        }


        /// @brief 交换缓冲区
        /// @param buffer
        void Swap(Buffer& buffer);

        /// @brief 确保缓冲区有足够的空间写入数据
        /// @param len
        void EnableWrite(std::size_t len);

        /// @brief 读取最多n个类型为T的值,并清除buffer中被读取的数据
        /// @tparam T 读取类型
        /// @param n 数量
        /// @return n个或少于n个类型为T的值组成的vector
        template <typename T>
        std::vector<T> Read(std::size_t n);

        /// @brief 获取可写缓冲区指针
        /// @return
        char *WriteStart();

        /// @brief 获取可读缓冲区指针
        /// @return
        const char *ReadStart() const;

        /// @brief 获取缓冲区剩余可用大小
        /// @return
        std::size_t Available() const;


    private:
        /// @brief 扩展缓冲区容量
        /// @param len 扩展长度
        void Expand(size_t len);

        /// @brief 移动数据到缓冲区开头
        void MoveData();

    private:
        std::size_t start_, end_;
        std::size_t capacity_;
        std::vector<char> buffer_; // 缓冲区
    };

    template <typename T>
    std::vector<T> Buffer::Read(std::size_t n)
    {
        std::vector<T> result;
        if (n <= 0)
        {
            assert(false);
            return result;
        }
        std::size_t readable_bytes = Size();
        std::size_t max_num = readable_bytes / sizeof(T);
        n = std::min(max_num, n);
        result.resize(n);
        char *dest = (char *)&result[0];
        std::copy(ReadStart(), ReadStart() + n * sizeof(T), dest);
        start_ += n * sizeof(T);
        return result;
    }
    */
}