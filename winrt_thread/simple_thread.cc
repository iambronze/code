#include "simple_thread.h"
#include "pch.h"

namespace base {

SimpleThread::SimpleThread()
    : thread_(), joined_(false),
      priority_(ThreadPriority::NORMAL) {
}

SimpleThread::SimpleThread(const ThreadPriority &priority)
    : priority_(priority),
      thread_(), joined_(false) {
}

SimpleThread::~SimpleThread() {
}

void SimpleThread::Start() {
  bool success;
  if (priority_ == ThreadPriority::NORMAL) {
    success = PlatformThread::Create(this, &thread_);
  } else {
    success = PlatformThread::CreateWithPriority(this,
                                                 &thread_,
                                                 priority_);
  }
}

void SimpleThread::Join() {
  PlatformThread::Join(thread_);
  joined_ = true;
}

void SimpleThread::ThreadMain() {
  Run();
}

DelegateSimpleThread::DelegateSimpleThread(Delegate *delegate)
    : SimpleThread(),
      delegate_(delegate) {
}

DelegateSimpleThread::DelegateSimpleThread(Delegate *delegate,
                                           const ThreadPriority &priority)
    : SimpleThread(priority),
      delegate_(delegate) {
}

DelegateSimpleThread::~DelegateSimpleThread() {
}

void DelegateSimpleThread::Run() {
  delegate_->Run();
  delegate_ = NULL;
}

}  // namespace base