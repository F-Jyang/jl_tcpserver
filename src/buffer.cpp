#include "buffer.h"

jl::Buffer::Buffer(std::size_t capacity) : start_(0),
                                           end_(0),
                                           capacity_(capacity),
                                           buffer_(capacity)
{
}

std::size_t jl::Buffer::Available() const
{
    assert(end_ >= start_ && end_ < buffer_.size());
    return capacity_ - (end_ - start_);
}

void jl::Buffer::Swap(Buffer &other)
{
    std::swap(start_, other.start_);
    std::swap(end_, other.end_);
    std::swap(capacity_, other.capacity_);
    buffer_.swap(other.buffer_);
}

void jl::Buffer::Consume(std::size_t len)
{
    assert(end_ >= start_ && end_ < buffer_.size());
    len = std::min(len, end_ - start_);
    start_ += len;
}

void jl::Buffer::EnableWrite(std::size_t len)
{
    if (Available() >= len)
    {
        if (capacity_ - end_ < len)
        {
            MoveData();
        }
    }
    else
    {
        Expand(static_cast<std::size_t>(len * 1.5));
    }
}

void jl::Buffer::Expand(size_t len)
{
    capacity_ += len;
    std::vector<char> new_buffer(capacity_, 0);
    std::copy(buffer_.begin() + start_, buffer_.begin() + end_, new_buffer.begin());
    start_ = 0;
    end_ = this->Size();
    buffer_.swap(new_buffer);
}

void jl::Buffer::MoveData()
{
    if (start_ > 0)
    {
        std::copy(buffer_.begin() + start_, buffer_.begin() + end_, buffer_.begin());
        end_ = this->Size();
        start_ = 0;
    }
}

void jl::Buffer::AppendEnd(std::size_t len)
{
    end_ += len;
    assert(end_ < buffer_.size());
}

void jl::Buffer::AppendStart(std::size_t len) {
    start_ += len;
    assert(start_ <= end_);
}

std::string jl::Buffer::ReadAsString(std::size_t n)
{
    n = std::min(n, Size());
    if (n <= 0)
        return "";
    std::string result(Start(), Start() + n);
    start_ += n;
    return result;
}

std::string jl::Buffer::ReadAll()
{
    return ReadAsString(Size());
}

void jl::Buffer::Clear()
{
    start_ = 0;
    end_ = 0;
    buffer_.clear();
}

size_t jl::Buffer::Size() const
{
    return end_ - start_;
}

//std::vector<char>& jl::Buffer::Data()
//{
//    return buffer_;
//}

char *jl::Buffer::End()
{
    assert(end_ >= start_ && end_ < buffer_.size());
    return &buffer_[end_];
}

const char *jl::Buffer::Start() const
{
    assert(end_ >= start_ && end_ < buffer_.size());
    return &buffer_[start_];
}

jl::Buffer::~Buffer()
{
    buffer_.clear();
}

void jl::Buffer::Append(const std::vector<char> &data)
{
    this->Append(data.data(), data.size());
}

void jl::Buffer::Append(const std::string &data)
{
    this->Append(data.data(), data.size());
}

// void jl::Buffer::Append(const Buffer &buffer)
// {
//     this->Append(buffer.Data(), buffer.Size());
// }

void jl::Buffer::Append(const void *data, std::size_t len)
{
    EnableWrite(len);
    const char *data_ptr = static_cast<const char *>(data);
    std::copy(data_ptr, data_ptr + len, End());
    end_ += len;
}
