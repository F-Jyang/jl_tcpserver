/// @file buffer.h
/// @brief 缓冲区类
/// @author Jyang.
/// @date 2026-1-17
/// @version 1.0

#pragma once

#include <vector>
#include <assert.h>
#include <asio/buffer.hpp>

constexpr std::size_t kDefaultCapacity = 2048;

namespace jl
{
    class Connection;
    class SslConnection;
    
    class Buffer
    {
        friend class Connection;
        friend class SslConnection;
    public:
        Buffer(std::size_t capacity = kDefaultCapacity);

        ~Buffer();

        /// @brief 添加一个类型T的数据到buffer
        /// @tparam T 要求为POD类型或基础数据类型
        /// @param data
        template <typename T, typename DecayT = std::decay_t<T>, typename = std::enable_if_t<std::is_fundamental_v<DecayT> || std::is_pod_v<DecayT>>>
        void Append(T &&data)
        {
            // static_assert(std::is_pod_v<DecayT>,
            // "should never fire");
            //     static_assert(std::is_fundamental_v<DecayT>,
            //       "should never firea");

            static_assert(!std::is_pointer_v<DecayT>, "Buffer::Append: T can't be pointer type."); // cpp17及17之前的bug，pointer会被识别为pod类型
            this->Append(std::addressof(data), sizeof(DecayT));
        }

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

        /// @brief 添加包含len个数据的指针到buffer
        /// @param data
        /// @param len
        void Append(const void *data, std::size_t len);

        /// @brief 添加vector<char>的数据到buffer
        /// @param data
        void Append(const std::vector<char> &data);

        /// @brief 添加string的数据到buffer
        /// @param data
        void Append(const std::string &data); // 追加数据到缓冲区

        // void Append(const Buffer &buffer); // 追加数据到缓冲区

        /// @brief 读取最多n个类型为T的值,并清除buffer中被读取的数据
        /// @tparam T 读取类型
        /// @param n 数量
        /// @return n个或少于n个类型为T的值组成的vector
        template <typename T>
        std::vector<T> Read(std::size_t n);

        /// @brief 读取n个char组成的string,buffer的长度<n时返回最大长度
        /// @param n
        /// @return
        std::string ReadAsString(std::size_t n);

        /// @brief 读取所有字节并返回string
        /// @param n
        /// @return
        std::string ReadAll();

        /// @brief 清空缓冲区
        void Clear();

        /// @brief 获取缓冲区大小
        /// @return
        size_t Size() const;

        /// @brief 获取缓冲区数据
        /// @return 缓冲区的首地址指针
        //std::vector<char>& Data();

        /// @brief 获取可写缓冲区指针
        /// @return
        char *End();

        /// @brief 获取可读缓冲区指针
        /// @return
        const char *Start() const;

        /// @brief 获取缓冲区剩余可用大小
        /// @return
        std::size_t Available() const;

        /// @brief 交换缓冲区
        /// @param buffer
        void Swap(Buffer &buffer);

        /// @brief 删除长度为len的缓冲区数据
        /// @param len
        void Consume(std::size_t len);

        /// @brief 确保缓冲区有足够的空间写入数据
        /// @param len
        void EnableWrite(std::size_t len);

    private:
        /// @brief 扩展缓冲区容量
        /// @param len 扩展长度
        void Expand(size_t len);

        /// @brief 移动数据到缓冲区开头
        void MoveData();

        /// @brief end+=len。更新end指针，用于asio::read/asio::write的回调函数中手动更新指针
        /// @param len
        void AppendEnd(std::size_t len);

        /// @brief start+=len。更新start指针，用于asio::read/asio::write的回调函数中手动更新指针
        /// @param len
        void AppendStart(std::size_t len);

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
        std::copy(Start(), Start() + n * sizeof(T), dest);
        start_ += n * sizeof(T);
        return result;
    }

}