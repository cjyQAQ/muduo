#pragma once

#include <memory>
#include <functional>
#include "Timestamp.h"

class Buffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;

using MessageCallback = std::function<void(const TcpConnectionPtr &conn,Buffer *buffer,Timestamp recvtime)>;

using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &,size_t)>;