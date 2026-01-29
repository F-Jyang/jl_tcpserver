#pragma once

#include <string>
#include <mutex>
#include <chrono>
#include <vector>

namespace jl
{
    namespace util
    {
        class IdGenerator
        {
        public:
            virtual std::int64_t GenerateId() = 0;

            virtual std::vector<std::int64_t> GenerateIds(std::size_t cnt) = 0;
        };

        class Snowflake : public IdGenerator
        {
            // 64位ID结构:
            /*
                |--- 1位符号位(0) ---|--- 41位时间戳 ---|--- 10位节点ID ---|--- 12位序列号 ---|
            */

            // 协议中的常量
            static constexpr std::int64_t kTwepoch = 1609459200000;
            static constexpr int kNodeIdBits = 10;
            static constexpr int kSequenceBits = 12;

            static constexpr std::int64_t kMaxNodeId = (1 << kNodeIdBits) - 1;     // 1023
            static constexpr std::int64_t kMaxSequence = (1 << kSequenceBits) - 1; // 4095

            static constexpr int kNodeIdShift = kSequenceBits;
            static constexpr int kTimestampShift = (kSequenceBits + kNodeIdBits);

            // 时钟回拨容忍度(ms)
            static constexpr std::int64_t kMaxBackwardMs = 5;

        public:
            explicit Snowflake(std::int64_t node_id) :
                node_id_(node_id),
                sequence_(0),
                last_timestamp_(-1)
            {
                if (node_id_ < 0 || node_id_ > kMaxNodeId)
                {
                    throw std::invalid_argument("Node ID out of range [0," + std::to_string(kMaxNodeId) + "]");
                }
            }

            /// @brief 生成下一个id
            std::int64_t GenerateId() override
            {
                std::lock_guard<std::mutex> lock(mutex_);
                std::int64_t timestamp = CurrentTimeMillis();
                // 时钟回拨检测
                if (timestamp < last_timestamp_)
                {
                    std::int64_t offset = last_timestamp_ - timestamp;
                    if (offset > kMaxBackwardMs)
                    {
                        throw std::runtime_error("Clock moved backwards by" + std::to_string(offset) + "ms, refusing to generate ID.");
                    }
                    // 小幅度回拨，等待追平
                    std::this_thread::sleep_for(std::chrono::milliseconds(offset));
                    timestamp = CurrentTimeMillis();
                }
                // 同一毫秒内
                if (timestamp == last_timestamp_)
                {
                    sequence_ = (sequence_ + 1) % kMaxSequence;
                    if (sequence_ == 0)
                    {
                        timestamp = WaitNextMills(timestamp);
                    }
                }
                else
                {
                    // 不同毫秒，序列化重复
                    sequence_ = 0;
                }
                last_timestamp_ = timestamp;

                // 组合
                return ((last_timestamp_ - kTwepoch) << kTimestampShift) |
                       (node_id_ << kNodeIdShift) |
                       sequence_;
            }

            /// @brief 批量生成id
            std::vector<std::int64_t> GenerateIds(std::size_t cnt) override
            {
                std::vector<std::int64_t> ids;
                std::lock_guard<std::mutex> lock(mutex_);
                while (cnt-- > 0)
                {
                    std::int64_t timestamp = CurrentTimeMillis();
                    // 时钟回拨检测
                    if (timestamp < last_timestamp_)
                    {
                        std::int64_t offset = last_timestamp_ - timestamp;
                        if (offset > kMaxBackwardMs)
                        {
                            throw std::runtime_error("Clock moved backwards by" + std::to_string(offset) + "ms, refusing to generate ID.");
                        }
                        // 小幅度回拨，等待追平
                        std::this_thread::sleep_for(std::chrono::milliseconds(offset));
                        timestamp = CurrentTimeMillis();
                    }
                    // 同一毫秒内
                    if (timestamp == last_timestamp_)
                    {
                        sequence_ = (sequence_ + 1) % kMaxSequence;
                        if (sequence_ == 0)
                        {
                            timestamp = WaitNextMills(timestamp);
                        }
                    }
                    else
                    {
                        // 不同毫秒，序列化重复
                        sequence_ = 0;
                    }
                    last_timestamp_ = timestamp;
                    std::int64_t id = ((last_timestamp_ - kTwepoch) << kTimestampShift) | (node_id_ << kNodeIdShift) | sequence_;
                    ids.emplace_back(id);
                }
                return ids;
            }

            std::int64_t NodeId() const
            {
                return node_id_;
            }

            // for debug
            struct Components
            {
                std::int64_t timestamp;
                std::int64_t node_id;
                std::int64_t sequence;
                std::chrono::system_clock::time_point time;
            };

            static Components Parse(std::int64_t id)
            {
                Components c;
                c.timestamp = (id >> kTimestampShift) + kTwepoch;
                c.node_id = (id >> kNodeIdShift) & kMaxNodeId;
                c.sequence = id & kMaxSequence;
                c.time = std::chrono::system_clock::time_point(std::chrono::milliseconds(c.timestamp));
                return c;
            }

        private:
            static std::int64_t CurrentTimeMillis()
            {
                return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            }

            static std::int64_t WaitNextMills(std::int64_t last_ts)
            {
                std::int64_t ts = CurrentTimeMillis();
                while (ts <= last_ts)
                {
                    ts = CurrentTimeMillis();
                }
                return ts;
            }

        private:
            const std::int64_t node_id_;
            std::int64_t sequence_;
            std::int64_t last_timestamp_;
            std::mutex mutex_;
        };


        template<typename Generator, typename... Args>
        inline IdGenerator*  MakeIdGenerator(Args&&... args)
        {
            return new Generator(std::forward<Args>(args)...);
        }
    }
}