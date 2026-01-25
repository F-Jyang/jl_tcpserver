### TODO:
#### 实现一个类似 asio::transfer_all 的函数，用于读取最大指定数量的字节
#### bug:server退出时需要所有的Connection->Close()
#### server添加asio::resolver，支持域名解析
#### 添加唯一id生成函数，雪花算法
#### ssl::context设置证书等函数实现、ssl连接测试
#### 添加更多例子
#### 添加性能测试

### Introduce
#### Connection
Connection、SslConnection是ConnectionTemplate的特化，ConnectionTemplate继承自 BaseConnection。通过对ConnectionTemplate的成员函数的特化可以实现两种连接的不同行为。

连接的生命周期一般是:

- Connection 是普通连接:
  - Acceptor->async_accept->Connection->Start->Read->OnRead->Write->OnWrite->Read...->Close
- SslConnection 是 ssl 连接
  - Acceptor->async_accept->SslConnection->Start->Handshake->OnHandshake->Read->OnRead->Write->OnWrite->Read...->Close

在acceptor调用async_accept接收到连接时，acceptor会调用OnConnEstablishCallback回调函数并传入Socket&&，使用Socket&& 创建Connection或SslConnection，并设置它们的OnReadCallback、OnWriteCallback、OnCloseCallback、OnTimeoutCallback等回调函数控制连接不同生命周期的行为。

### perf

### Asio的一些坑

代码版本为 asio v1.36.0

#### buffer
asio中有 asio::is_dynamic_buffer_v2<>、asio::is_dynamic_buffer_v1<> 两个模板用来判断 buffer 的类型。该两种类型的buffer的不同取决于其成员函数。asio中对buffer_v1、buffer_v2的接口要求不同。例如 [buffer_v2的定义](`https://think-async.com/Asio/asio-1.36.0/doc/asio/reference/DynamicBuffer_v2.html`)。

推荐使用asio::streambuf，asio::streambuf 实现了buffer_v2相关的接口，其代码并不复杂。
其中官方文档中对asio::streambuf的读写使用的是std::ostream和std::istream，会自动管理asio::streambuf中相关的读、写等指针。如果不使用官方文档中的方法，而是将使用asio::streambuf::data()等方法进行读写，需要自己管理指针(如读完部分内存后需要调用consum()消耗内存，向buffer写入部分内存后调用commit提交内存)。

##### const_buffer 与 mutable_buffer

const_buffer 和 mutable_buffer 的定义非常简单，两者都是只保存数据的指针和size，并不保存数据本身。两者的区别在于，const_buffer 中的指针是 const void* 类型，mutable_buffer 的指针是 void* 类型。

const_buffer 实现如下:

```cpp
class const_buffer
{
public:
  /// Construct an empty buffer.
  const_buffer() noexcept
    : data_(0),
      size_(0)
  {
  }

  /// Construct a buffer to represent a given memory range.
  const_buffer(const void* data, std::size_t size) noexcept
    : data_(data),
      size_(size)
  {
  }

  /// Construct a non-modifiable buffer from a modifiable one.
  const_buffer(const mutable_buffer& b) noexcept
    : data_(b.data()),
      size_(b.size())
#if defined(ASIO_ENABLE_BUFFER_DEBUGGING)
      , debug_check_(b.get_debug_check())
#endif // ASIO_ENABLE_BUFFER_DEBUGGING
  {
  }

  const asio::detail::function<void()>& get_debug_check() const
  {
    return debug_check_;
  }
#endif // ASIO_ENABLE_BUFFER_DEBUGGING

  /// Get a pointer to the beginning of the memory range.
  const void* data() const noexcept
  {
#if defined(ASIO_ENABLE_BUFFER_DEBUGGING)
    if (size_ && debug_check_)
      debug_check_();
#endif // ASIO_ENABLE_BUFFER_DEBUGGING
    return data_;
  }

  /// Get the size of the memory range.
  std::size_t size() const noexcept
  {
    return size_;
  }

  /// Move the start of the buffer by the specified number of bytes.
  const_buffer& operator+=(std::size_t n) noexcept
  {
    std::size_t offset = n < size_ ? n : size_;
    data_ = static_cast<const char*>(data_) + offset;
    size_ -= offset;
    return *this;
  }

private:
  const void* data_;
  std::size_t size_;

#if defined(ASIO_ENABLE_BUFFER_DEBUGGING)
  asio::detail::function<void()> debug_check_;
#endif // ASIO_ENABLE_BUFFER_DEBUGGING
};

```

mutable_buffer实现如下:

```cpp
class mutable_buffer
{
public:
  /// Construct an empty buffer.
  mutable_buffer() noexcept
    : data_(0),
      size_(0)
  {
  }

  /// Construct a buffer to represent a given memory range.
  mutable_buffer(void* data, std::size_t size) noexcept
    : data_(data),
      size_(size)
  {
  }

#if defined(ASIO_ENABLE_BUFFER_DEBUGGING)
  mutable_buffer(void* data, std::size_t size,
      asio::detail::function<void()> debug_check)
    : data_(data),
      size_(size),
      debug_check_(debug_check)
  {
  }

  const asio::detail::function<void()>& get_debug_check() const
  {
    return debug_check_;
  }
#endif // ASIO_ENABLE_BUFFER_DEBUGGING

  /// Get a pointer to the beginning of the memory range.
  void* data() const noexcept
  {
#if defined(ASIO_ENABLE_BUFFER_DEBUGGING)
    if (size_ && debug_check_)
      debug_check_();
#endif // ASIO_ENABLE_BUFFER_DEBUGGING
    return data_;
  }

  /// Get the size of the memory range.
  std::size_t size() const noexcept
  {
    return size_;
  }

  /// Move the start of the buffer by the specified number of bytes.
  mutable_buffer& operator+=(std::size_t n) noexcept
  {
    std::size_t offset = n < size_ ? n : size_;
    data_ = static_cast<char*>(data_) + offset;
    size_ -= offset;
    return *this;
  }

private:
  void* data_;
  std::size_t size_;

#if defined(ASIO_ENABLE_BUFFER_DEBUGGING)
  asio::detail::function<void()> debug_check_;
#endif // ASIO_ENABLE_BUFFER_DEBUGGING
};

```

##### asio::streambuf

asio::streambuf 实现了 dynamic_buffer_v2 相关的接口。构造函数传入 max_size 指定最大的缓冲区大小。可以使用 const_buffers_type streambuf::data() 拿到输入序列，使用 mutable_buffers_type streambuf::prepare(std::size_t) 得到输出序列，调用prepare时会执行类似muduo中ensureWritableBytes的操作，但是会超过max_size_时会抛出 std::length_error 异常 。关于 dynamic_buffer_v2 相关的其他接口参考文档。

asio::streambuf的官方demo，读写可以使用 <<、>> 流操作符

write:
```cpp
// Writing directly from an streambuf to a socket:
asio::streambuf b;
std::ostream os(&b);
os << "Hello, World!\n";

// try sending some data in input sequence
size_t n = sock.send(b.data());

b.consume(n); // sent data is removed from input sequence
```

read:
```cpp
// Reading from a socket directly into a streambuf:
asio::streambuf b;

// reserve 512 bytes in output sequence
asio::streambuf::mutable_buffers_type bufs = b.prepare(512);

size_t n = sock.receive(bufs);

// received data is "committed" from output sequence to input sequence
b.commit(n);

std::istream is(&b);
std::string s;
is >> s;

```

asio::streambuf 实现如下:

```cpp
typedef basic_streambuf<> streambuf;

#if defined(GENERATING_DOCUMENTATION)
template <typename Allocator = std::allocator<char>>
#else
template <typename Allocator>
#endif
class basic_streambuf
  : public std::streambuf,
    private noncopyable
{
public:
#if defined(GENERATING_DOCUMENTATION)
  /// The type used to represent the input sequence as a list of buffers.
  typedef implementation_defined const_buffers_type;

  /// The type used to represent the output sequence as a list of buffers.
  typedef implementation_defined mutable_buffers_type;
#else
  typedef const_buffer const_buffers_type;
  typedef mutable_buffer mutable_buffers_type;
#endif

  /// Construct a basic_streambuf object.
  /**
   * Constructs a streambuf with the specified maximum size. The initial size
   * of the streambuf's input sequence is 0.
   */
  explicit basic_streambuf(     // 构造函数，指定 max_size_
      std::size_t maximum_size = (std::numeric_limits<std::size_t>::max)(),
      const Allocator& allocator = Allocator())
    : max_size_(maximum_size),
      buffer_(allocator)
  {
    std::size_t pend = (std::min<std::size_t>)(max_size_, buffer_delta);
    buffer_.resize((std::max<std::size_t>)(pend, 1));
    setg(&buffer_[0], &buffer_[0], &buffer_[0]);
    setp(&buffer_[0], &buffer_[0] + pend);
  }

  /// Get the size of the input sequence.
  /**
   * @returns The size of the input sequence. The value is equal to that
   * calculated for @c s in the following code:
   * @code
   * size_t s = 0;
   * const_buffers_type bufs = data();
   * const_buffers_type::const_iterator i = bufs.begin();
   * while (i != bufs.end())
   * {
   *   const_buffer buf(*i++);
   *   s += buf.size();
   * }
   * @endcode
   */
  std::size_t size() const noexcept
  {
    return pptr() - gptr();
  }

  /// Get the maximum size of the basic_streambuf.
  /**
   * @returns The allowed maximum of the sum of the sizes of the input sequence
   * and output sequence.
   */
  std::size_t max_size() const noexcept
  {
    return max_size_;
  }

  /// Get the current capacity of the basic_streambuf.
  /**
   * @returns The current total capacity of the streambuf, i.e. for both the
   * input sequence and output sequence.
   */
  std::size_t capacity() const noexcept
  {
    return buffer_.capacity();
  }

  /// Get a list of buffers that represents the input sequence.
  /**
   * @returns An object of type @c const_buffers_type that satisfies
   * ConstBufferSequence requirements, representing all character arrays in the
   * input sequence.
   *
   * @note The returned object is invalidated by any @c basic_streambuf member
   * function that modifies the input sequence or output sequence.
   */
  const_buffers_type data() const noexcept
  {
    return asio::buffer(asio::const_buffer(gptr(),
          (pptr() - gptr()) * sizeof(char_type)));
  }

  /// Get a list of buffers that represents the output sequence, with the given
  /// size.
  /**
   * Ensures that the output sequence can accommodate @c n characters,
   * reallocating character array objects as necessary.
   *
   * @returns An object of type @c mutable_buffers_type that satisfies
   * MutableBufferSequence requirements, representing character array objects
   * at the start of the output sequence such that the sum of the buffer sizes
   * is @c n.
   *
   * @throws std::length_error If <tt>size() + n > max_size()</tt>.
   *
   * @note The returned object is invalidated by any @c basic_streambuf member
   * function that modifies the input sequence or output sequence.
   */
  mutable_buffers_type prepare(std::size_t n)
  {
    reserve(n);
    return asio::buffer(asio::mutable_buffer(
          pptr(), n * sizeof(char_type)));
  }

  /// Move characters from the output sequence to the input sequence.
  /**
   * Appends @c n characters from the start of the output sequence to the input
   * sequence. The beginning of the output sequence is advanced by @c n
   * characters.
   *
   * Requires a preceding call <tt>prepare(x)</tt> where <tt>x >= n</tt>, and
   * no intervening operations that modify the input or output sequence.
   *
   * @note If @c n is greater than the size of the output sequence, the entire
   * output sequence is moved to the input sequence and no error is issued.
   */
  void commit(std::size_t n)
  {
    n = std::min<std::size_t>(n, epptr() - pptr());
    pbump(static_cast<int>(n));
    setg(eback(), gptr(), pptr());
  }

  /// Remove characters from the input sequence.
  /**
   * Removes @c n characters from the beginning of the input sequence.
   *
   * @note If @c n is greater than the size of the input sequence, the entire
   * input sequence is consumed and no error is issued.
   */
  void consume(std::size_t n)
  {
    if (egptr() < pptr())
      setg(&buffer_[0], gptr(), pptr());
    if (gptr() + n > pptr())
      n = pptr() - gptr();
    gbump(static_cast<int>(n));
  }

protected:
  enum { buffer_delta = 128 };

  /// Override std::streambuf behaviour.
  /**
   * Behaves according to the specification of @c std::streambuf::underflow().
   */
  int_type underflow()      // 不知道干嘛用的函数
  {
    if (gptr() < pptr())
    {
      setg(&buffer_[0], gptr(), pptr());
      return traits_type::to_int_type(*gptr());
    }
    else
    {
      return traits_type::eof();
    }
  }

  /// Override std::streambuf behaviour.
  /**
   * Behaves according to the specification of @c std::streambuf::overflow(),
   * with the specialisation that @c std::length_error is thrown if appending
   * the character to the input sequence would require the condition
   * <tt>size() > max_size()</tt> to be true.
   */
  int_type overflow(int_type c)         // 不知道干嘛用的函数
  {
    if (!traits_type::eq_int_type(c, traits_type::eof()))
    {
      if (pptr() == epptr())
      {
        std::size_t buffer_size = pptr() - gptr();
        if (buffer_size < max_size_ && max_size_ - buffer_size < buffer_delta)
        {
          reserve(max_size_ - buffer_size);
        }
        else
        {
          reserve(buffer_delta);
        }
      }

      *pptr() = traits_type::to_char_type(c);
      pbump(1);
      return c;
    }

    return traits_type::not_eof(c);
  }

  void reserve(std::size_t n)       // 扩展缓冲区，类似muduo的操作，但是加了max_size_的限制
  {
    // Get current stream positions as offsets.
    std::size_t gnext = gptr() - &buffer_[0];
    std::size_t pnext = pptr() - &buffer_[0];
    std::size_t pend = epptr() - &buffer_[0];

    // Check if there is already enough space in the put area.
    if (n <= pend - pnext)
    {
      return;
    }

    // Shift existing contents of get area to start of buffer.
    if (gnext > 0)
    {
      pnext -= gnext;
      std::memmove(&buffer_[0], &buffer_[0] + gnext, pnext);
    }

    // Ensure buffer is large enough to hold at least the specified size.
    if (n > pend - pnext)
    {
      if (n <= max_size_ && pnext <= max_size_ - n)
      {
        pend = pnext + n;
        buffer_.resize((std::max<std::size_t>)(pend, 1));
      }
      else
      {
        std::length_error ex("asio::streambuf too long");
        asio::detail::throw_exception(ex);
      }
    }

    // Update stream positions.
    setg(&buffer_[0], &buffer_[0], &buffer_[0] + pnext);
    setp(&buffer_[0] + pnext, &buffer_[0] + pend);
  }

private:
  std::size_t max_size_;
  std::vector<char_type, Allocator> buffer_;

  // Helper function to get the preferred size for reading data.
  friend std::size_t read_size_helper(
      basic_streambuf& sb, std::size_t max_size)
  {
    return std::min<std::size_t>(
        std::max<std::size_t>(512, sb.buffer_.capacity() - sb.size()),
        std::min<std::size_t>(max_size, sb.max_size() - sb.size()));
  }
};
```

#### 读写相关api

##### socket的成员函数api

对于读写相关的api，最常见的是 socket.async_read_some、socket.async_write_some。

这些api将数据读/写入传入的buffer中，buffer需要自己进行维护。

##### asio::xxxx 的api

比如函数: asio::async_read、asio::async_write、asio::async_read_until，这些函数是对socket的成员函数api的组合调用。

需要传入的buffer一般要求buffer_v1、buffer_v2，不能简单传入一个char*和len。这些函数的调用会自动管理buffer的指针，比如使用 aiso::streambuf 传入 asio::async_read 后无需手动 buffer::commit()，传入 asio::async_write 后无需手动调用buffer::consum()。一般来说会比直接调用socket的成员函数更方便、更安全。(ps:对于 asio::async_read 读取到的数据如果不用 std::ostream 的方式读取，可能需要手动调用  asio::streambuf::consum)

#### strand
add_compile_definitions(-D ASIO_NO_DEPRECATED) # 禁用asio废弃api
##### asio::io_context::strand
属于旧模型中的api，已经不被推荐使用，其中的post、wrap等成员函数已经被废弃，且使用该类与asio::bind_executor()等函数结合实现异步操作时可能会出bug，导致异步操作的回调函数不会被执行。

##### asio::strand<> 模板类
新模型的api，将io_context变成了模板参数进行解耦和，是推荐使用的api。 asio::strand\<asio::io_context::executor_type\>模板类，结合asio::post、asio::bind_executor() 实现异步回调函数的关联到strand，从而实现异步操作、异步操作的回调函数的串行。 
```cpp
asio::strand<asio::io_context::executor_type> strand_(std::make_strand(io_context_));
```
##### asio::post 保证异步操作串行 📌
post只保证post到相关executor的函数是串行的，如果函数中有类似async_write之类注册异步操作的函数，post并不保证async_write的回调函数也是串行的。
```cpp
inline void ConnectionTemplate<SocketType>::Read(std::size_t max_bytes)
{
    auto self = shared_from_this();
    asio::post(io_strand_, [=]()  // 由io_strand_保证串行
    { 
        bool expected = false;
        if (read_in_progress_.compare_exchange_strong(expected, true)) { // 保证同一时间只注册一个read操作
            asio::async_read(this->socket_, this->read_buffer_, 
                asio::transfer_at_least(1),
                [=](const std::error_code& ec, size_t bytes_transferred) // 并不能保证该回调串行执行
                { /*...*/ } 
            );
        }
        }
    );
}
```
如果需要保证异步操作的回调函数也是串行，则需要对异步操作的回调函数使用asio::bind_executor()，绑定一个asio::strand<>之类的东西。
```cpp
inline void ConnectionTemplate<SocketType>::Read(std::size_t max_bytes)
{
    auto self = shared_from_this();
    asio::post(io_strand_, [=]()  // 由io_strand_保证串行
    { 
        bool expected = false;
        if (read_in_progress_.compare_exchange_strong(expected, true)) {
            asio::async_read(this->socket_, this->read_buffer_, 
                asio::transfer_at_least(1),
                asio::bind_executor(io_strand_, 
                    [=](const std::error_code& ec, size_t bytes_transferred) // 由 io_strand_ 保证串行
                    { /*...*/ }
                ) 
            );
        }
        }
    );
}
```

#### 一些实践技巧

##### async_read/async_write的注册

保证同一时间内**最多只注册了一个async_read和一个async_write操作**，避免同时注册多个相同的操作。📌

比如：同时调用asio::post 注册了两个 async_read 操作，由于async_read操作的异步调用函数在收到数据时才调用，因收到一个数据时，两个async_read的回调是不确定的。

对于 async_read 的解决方法，使用一个标记正在读，读结束后才允许注册下一个async_read操作。
```cpp
inline void ConnectionTemplate<SocketType>::Read(std::size_t max_bytes)
{
    auto self = shared_from_this();
    asio::post(io_strand_, [=]()  // 由io_strand_保证串行
    { 
        bool expected = false;
        if (read_in_progress_.compare_exchange_strong(expected, true)) { // 保证当前async_read结束前不会注册第二个async_read操作
            asio::async_read(this->socket_, this->read_buffer_, 
                asio::transfer_at_least(1),
                asio::bind_executor(io_strand_,  // 异步函数不会立刻执行
                	bool expected = true; 
				    read_in_progress_.compare_exchange_strong(expected, false); // 当前回调结束，允许再次注册async_read
				    self->OnRead(ec, bytes_transferred);
                    
                ) 
            );
        }
        }
    );
}
```

对于async_write，保证同一时间只有一个async_write被注册的方法是使用队列。队列为空则没有正在写，允许注册async_write。队列不为空则存在async_write，不允许注册async_write：
```cpp
template<typename SocketType>
inline void ConnectionTemplate<SocketType>::Write(const void* data, std::size_t n)
{
    auto self = shared_from_this();
    std::string copy(static_cast<const char*>(data), n); // 拷贝数据，传入lambda中避免data失效
    PostTask([=]() {
        const bool write_in_progress = !this->send_queue_.empty(); // 判断是否正在写
        this->send_queue_.emplace(copy);
        if (!write_in_progress) {
            self->DoWrite(); // 如果当前没有写，就注册一个async_write操作
        }}
    );
}

template<typename SocketType>
inline void ConnectionTemplate<SocketType>::DoWrite()
{
    std::size_t n = this->send_queue_.front().size();
    auto self = shared_from_this();
    asio::async_write(socket_,
        asio::buffer(this->send_queue_.front()),
        asio::transfer_exactly(n),
        asio::bind_executor(io_strand_, 
            [=](const std::error_code& ec, size_t bytes_transferred)
            {
                self->OnWrite(ec, bytes_transferred);   // 写回调
                if (!ec) {
                    this->send_queue_.pop();
                    if (!this->send_queue_.empty()) {   // 不为空，继续写
                        self->DoWrite();
                    }
                }
            }
        )
    );
}

```
##### 使用多个strand分发读写

使用多个 strand 分别关联 async_read 和 async_write 操作，使得读写同步进行。这种方式的实现难度更大、更容易出现bug，同时分发读写操作也会增加一定的延迟，性能不一定会比单个 strand 分发读写操作要高。还没见过有这种操作的例子或开源项目，不推荐这种操作。