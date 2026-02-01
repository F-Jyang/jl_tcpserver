## TODO:
### connection添加超时io函数
### 实现一个类似 asio::transfer_all 的函数，用于读取最大指定数量的字节
### bug:server退出时需要所有的Connection->Close()
### server添加asio::resolver，支持域名解析
### 添加唯一id生成函数，雪花算法
### ssl::context设置证书等函数实现、ssl连接测试
### 添加更多例子
### 添加性能测试

## Introduce
### Server

Server使用的是 `单asio::io_context-多线程` 的方式，只有一个 `asio::io_context`，但是有多个线程在运行它。各个连接的io操作通过 `asio::strand<>` 来保证串行执行，避免多个线程同时操作一个连接导致的竞态条件。

Server 中有一个Acceptor，用于监听端口并接受连接。Acceptor 会在 io_context 中运行，当有新连接时，会调用 `OnConnEstablishCallback(net::socket&&)` 回调函数，将新连接的Socket&&传入回调函数，回调函数中可以根据需要创建Connection、SSLConnection，或者直接使用socket进行异步操作。

`socket` 创建时已经绑定 `asio::strand<>` ，从而确保在其异步io操作是串行执行的。

```cpp
void jl::Acceptor::DoAccept()
{
  // ...
  // socket创建时绑定 asio::strand<> 确保在 io_context 中串行执行
  auto socket_ptr = std::make_shared<net::socket>(asio::make_strand(ioct_));
  auto self(shared_from_this()); 
  acceptor_.async_accept(*socket_ptr, [socket_ptr, self](const std::error_code& ec) {
      self->OnAccept(ec, std::move(*socket_ptr));
      }
  );
}
```
对于其他如 `Timer` 等需要与 `socket` 绑定的操作，也需要注意：
1. 通过创建时绑定 `socket` 的 `asio::strand<>` 
2. 通过 `asio::post(socket.get_executor(),function)`  将操作提交到 `socket` 绑定的 `asio::strand<>` 中执行

等方式保证串行，否则可能会导致竞态条件。


### Connection
Connection、SSLConnection实现接口IConnection。

连接的生命周期一般是:

- Connection 是普通连接:
  - Acceptor->async_accept->Connection->Read->OnRead->Write->OnWrite->Read...->Close
- SslConnection 是 ssl 连接
  - Acceptor->async_accept->SSLConnection->Handshake->OnHandshake->Read->OnRead->Write->OnWrite->Read...->Close

在acceptor调用async_accept接收到连接时，acceptor会调用OnConnEstablishCallback回调函数并传入Socket&&，使用Socket&& 创建Connection或SSLConnection，并设置它们的OnReadCallback、OnWriteCallback、OnCloseCallback、OnTimeoutCallback等回调函数控制连接不同生命周期的行为。

注意：
Connection作为类的成员变量时，需要注意Connection的回调函数不能捕获类的 shared_from_this() 指针，否则会有Connection和类循环引用从而导致内存泄漏的风险。如果Connection的回调函数需要捕获类的 shared_from_this() 指针，需要使用 std::weak_ptr 来避免循环引用。

### Timer
Timer 是一个基于 asio::steady_timer 实现的定时器，用于在指定时间后调用回调函数。

注意：
Timer作为类的成员变量时，需要注意Timer的回调函数不能捕获类的 shared_from_this() 指针，否则会有Timer和类循环引用从而导致内存泄漏的风险。作为成员变量时需要捕获类的 shared_from_this() 时，可以使用 std::weak_ptr 来避免循环引用。
```cpp
class HttpSession : public std::enable_shared_from_this<HttpSession>{
  // ...... 
  std::shared_ptr<jl::Timer> timer_;
  //......
};

void HttpSession::Start()
{
  // ......
  // 使用std::weak_ptr<> 避免循环引用导致的内存泄漏
  std::weak_ptr<HttpSession> weak = shared_from_this();
  timer_->SetCallback([=]() {
      auto self = weak.lock();
      if (self) {
          self->OnTimeout();
      };
      });
  timer_->Wait(5000);
  // ......
}
```

## perf
 