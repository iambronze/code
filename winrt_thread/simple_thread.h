#ifndef SIMPLE_THREAD_H_
#define SIMPLE_THREAD_H_

#include <stddef.h>

#include <queue>
#include <string>
#include <vector>

#include "platform_thread.h"

namespace base {

class SimpleThread : public PlatformThread::Delegate {
public:
  SimpleThread();
  SimpleThread(const ThreadPriority &priority);

  ~SimpleThread() override;

  virtual void Start();
  virtual void Join();

  // Subclasses should override the Run method.
  virtual void Run() = 0;

  // Return True if Join() has evern been called.
  bool HasBeenJoined() { return joined_; }

  // Overridden from PlatformThread::Delegate:
  void ThreadMain() override;

private:
  const ThreadPriority priority_;
  PlatformThreadHandle thread_;
  bool joined_;
};

class DelegateSimpleThread : public SimpleThread {
public:
  class Delegate {
  public:
    Delegate() {}
    virtual ~Delegate() {}
    virtual void Run() = 0;
  };

  DelegateSimpleThread(Delegate *delegate);
  DelegateSimpleThread(Delegate *delegate,
                       const ThreadPriority &priority);

  ~DelegateSimpleThread() override;
  void Run() override;

private:
  Delegate *delegate_;
};

}  // namespace base

#endif  // SIMPLE_THREAD_H_