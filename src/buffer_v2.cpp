//#include "buffer.h"
//
///*
//jl::Buffer::Buffer(std::size_t capacity) :
//    start_(0),
//    end_(0),
//    capacity_(capacity),
//    buffer_(capacity)
//{
//}
//
//std::size_t jl::Buffer::Available() const
//{
//    assert(end_ >= start_ && end_ < buffer_.size());
//    return capacity_ - (end_ - start_);
//}
//
//void jl::Buffer::Swap(Buffer &other)
//{
//    std::swap(start_, other.start_);
//    std::swap(end_, other.end_);
//    std::swap(capacity_, other.capacity_);
//    buffer_.swap(other.buffer_);
//}
//
//asio::const_buffer jl::Buffer::data() const
//{
//    return asio::const_buffer(ReadStart(), size()) ;
//}
//
//void jl::Buffer::consume(std::size_t len)
//{
//    assert(end_ >= start_ && end_ < buffer_.size());
//    len = std::min(len, end_ - start_);
//    start_ += len;
//}
//
//void jl::Buffer::commit(std::size_t len)
//{
//    end_ += len;
//}
//
//std::size_t jl::Buffer::size() const
//{
//    return end_ - start_;
//}
//
//std::size_t jl::Buffer::max_size() const
//{
//    return buffer_.size();
//}
//
//asio::mutable_buffer jl::Buffer::prepare(std::size_t len)
//{
//    EnableWrite(len);
//    return asio::mutable_buffer(WriteStart(), len);
//}
//
//void jl::Buffer::EnableWrite(std::size_t len)
//{
//    if (Available() >= len)
//    {
//        if (capacity_ - end_ < len)
//        {
//            MoveData();
//        }
//    }
//    else
//    {
//        Expand(static_cast<std::size_t>(len * 1.5));
//    }
//}
//
//void jl::Buffer::Expand(size_t len)
//{
//    capacity_ += len;
//    std::vector<char> new_buffer(capacity_, 0);
//    std::copy(buffer_.begin() + start_, buffer_.begin() + end_, new_buffer.begin());
//    start_ = 0;
//    end_ = this->size();
//    buffer_.swap(new_buffer);
//}
//
//void jl::Buffer::MoveData()
//{
//    if (start_ > 0)
//    {
//        std::copy(buffer_.begin() + start_, buffer_.begin() + end_, buffer_.begin());
//        end_ = this->size();
//        start_ = 0;
//    }
//}
//
//char *jl::Buffer::WriteStart()
//{
//    assert(end_ >= start_ && end_ < buffer_.size());
//    return &buffer_[end_];
//}
//
//const char *jl::Buffer::ReadStart() const
//{
//    assert(end_ >= start_ && end_ < buffer_.size());
//    return &buffer_[start_];
//}
//
//jl::Buffer::~Buffer()
//{
//    buffer_.clear();
//}
//*/
//
//jl::Buffer::Buffer(size_t initial_capacity):
//    buffer_(initial_capacity),
//    max_size_(std::max(initial_capacity,kDefaultMaxSize))
//{
//}
//
//jl::Buffer::Buffer(Buffer&& other)
//{
//    std::swap(buffer_, other.buffer_);
//    std::swap(start_, other.start_);
//    std::swap(end_, other.end_);
//    std::swap(max_size_, other.max_size_);
//}
//
//jl::Buffer::Buffer(const Buffer& other)
//{
//    buffer_ = other.buffer_;
//    start_ = other.start_;
//    end_ = other.end_;
//    max_size_ = other.max_size_;
//}
//
//size_t jl::Buffer::size() const noexcept
//{
//    return end_ - start_;
//}
//
//size_t jl::Buffer::max_size() const noexcept
//{
//    return max_size_;
//}
//
//void jl::Buffer::set_max_size(size_t size)
//{
//    assert(size < buffer_.max_size());
//    max_size_ = size;
//}
//
//size_t jl::Buffer::capacity() const noexcept
//{
//    return buffer_.capacity();
//}
//
//jl::Buffer::const_buffers_type jl::Buffer::data(size_t pos, size_t n) const
//{
//    const_buffers_type result;
//    if (pos > size()) return result;
//    size_t available = size() - pos;
//    size_t readable_bytes = (n < available) ? n : available;
//    result.emplace_back(read_start() + pos, readable_bytes);
//    return result;
//}
//
//jl::Buffer::mutable_buffers_type jl::Buffer::data(size_t pos, size_t n)
//{
//    EnableWrite(pos + n);
//    mutable_buffers_type result;
//    result.emplace_back(read_start() + pos, n);
//    return result;
//}
//
//
//void jl::Buffer::grow(size_t n)
//{
//    assert(n >= 0);
//    EnableWrite(n);
//    end_ += n;
//}
//
//void jl::Buffer::shrink(size_t n)
//{
//    assert(n >= 0);
//    n = std::min(n, size());
//    end_ -= n;
// }
//
//void jl::Buffer::consume(size_t n)
//{
//    assert(n >= 0);
//    n = std::min(n, size());
//    start_ += n;
//}
//
//const char* jl::Buffer::data() const noexcept
//{
//    return read_start();
//}
//
//char* jl::Buffer::data() noexcept
//{
//    return &buffer_[start_];
//}
//
//void jl::Buffer::clear() noexcept
//{
//    start_ = 0;
//    end_ = 0;
//}
//
//std::size_t jl::Buffer::available() const
//{
//    return capacity() - size();
//}
//
//const char* jl::Buffer::read_start() const
//{
//    return &buffer_[start_];
//}
//
//char* jl::Buffer::write_start()
//{
//    return &buffer_[end_];
//}
//
//void jl::Buffer::EnableWrite(std::size_t len)
//{
//    if (available() >= len)
//    {
//        if (capacity() - end_ < len)
//        {
//            MoveData();
//        }
//    }
//    else
//    {
//        Expand(len);
//    }
//}
//
//void jl::Buffer::Expand(size_t len)
//{
//    if (capacity() + len > max_size_) {
//        throw std::length_error("Buffer: exceeds max_size");
//    }
//    std::vector<char> new_buffer(capacity() + len, 0);
//    std::copy(buffer_.begin() + start_, buffer_.begin() + end_, new_buffer.begin());
//    buffer_.swap(new_buffer);
//    end_ = this->size();
//    start_ = 0;
//}
//
//void jl::Buffer::MoveData()
//{
//    if (start_ > 0)
//    {
//        std::copy(buffer_.begin() + start_, buffer_.begin() + end_, buffer_.begin());
//        end_ = this->size();
//        start_ = 0;
//    }
//}
