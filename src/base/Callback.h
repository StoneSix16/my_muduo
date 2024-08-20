#pragma once

#include <memory>
#include <functional>

class Buffer;
class EventLoop;
class TcpConnection;
class Timestamp;

template<typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f) {
//   if (false)
//   {
//     implicit_cast<From*, To*>(0);
//   }

#ifndef NDEBUG
  assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
#endif
  return ::std::static_pointer_cast<To>(f);
}

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

using ConnectionCallback = std::function<void(const TcpConnectionPtr &)>;
using CloseCallback = std::function<void(const TcpConnectionPtr &)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr &)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr &, size_t)>;
using MessageCallback = std::function<void (const TcpConnectionPtr&, Buffer*, Timestamp)>;
using TimerCallback = std::function<void()> ;

using EventCallback = std::function<void()>;
using ReadEventCallback = std::function<void(Timestamp)>;

// using MessageCallback = std::function<void(const TcpConnectionPtr &,
//                                            Buffer *,
//                                            Timestamp)>;