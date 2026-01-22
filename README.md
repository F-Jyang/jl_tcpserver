### TODO:
#### Connection需要使用template class、type traits等在封装一层，除了Close、Start外的其它函数可以复用
#### 使用Read(n) 实现ReadUntil
#### bug:server退出时需要所有的Connection->Close()
#### server添加asio::resolver，支持域名解析
#### 添加唯一id生成函数，雪花算法
#### ssl::context设置证书等函数实现、ssl连接测试
#### 添加更多例子
#### 添加性能测试

### Introduce
#### Connection
Connection、SslConnection继承自 BaseConnection。

两种连接通过重写BaseConntion的Read、OnRead、Write、OnWrite等部分虚函数，实现不同的行为。
连接的生命周期一般是:

- Connection 是普通连接:
  - Acceptor->async_accept->Connection->Start->Read->OnRead->Write->OnWrite->Read...->Close
- SslConnection 是 ssl 连接
  - Acceptor->async_accept->SslConnection->Start->Handshake->OnHandshake->Read->OnRead->Write->OnWrite->Read...->Close

在acceptor调用async_accept接收到连接时，acceptor会调用OnConnEstablishCallback回调函数并传入Socket&&，使用Socket&& 创建Connection或SslConnection，并设置它们的OnReadCallback、OnWriteCallback、OnCloseCallback、OnTimeoutCallback等回调函数控制连接不同生命周期的行为。

### perf