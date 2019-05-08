#ifndef PLATFORM_THREAD_H_
#define PLATFORM_THREAD_H_

#include <stddef.h>
#include <stdint.h>
#include <windows.h>

namespace base {

typedef HANDLE PlatformThreadHandle;

enum class ThreadPriority : int {
  BACKGROUND,
  NORMAL,
  REALTIME_AUDIO,
};

class PlatformThread {
public:
  class Delegate {
  public:
    virtual void ThreadMain() = 0;

  protected:
    virtual ~Delegate() {}
  };

  static void Sleep(uint32_t milliseconds);

  static bool Create(Delegate *delegate,
                     PlatformThreadHandle *thread_handle) {
    return CreateWithPriority(delegate, thread_handle,
                              ThreadPriority::NORMAL);
  }

  static bool CreateWithPriority(Delegate *delegate,
                                 PlatformThreadHandle *thread_handle,
                                 ThreadPriority priority);

  static void Join(PlatformThreadHandle thread_handle);

};

}  // namespace base

#endif  // PLATFORM_THREAD_H_