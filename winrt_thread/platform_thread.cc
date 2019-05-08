#include <xutility>
#include <Windows.Foundation.h>
#include <Windows.System.Threading.h>
#include <wrl/event.h>
#include <stdio.h>
#include <Objbase.h>
#include "platform_thread.h"
#include "pch.h"

using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::System::Threading;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

namespace base {

namespace {

static WorkItemPriority GetWorkItemPriority(ThreadPriority priority) {
  ABI::Windows::System::Threading::WorkItemPriority work_priority
      = ABI::Windows::System::Threading::WorkItemPriority_Normal;
  if (priority == ThreadPriority::BACKGROUND) {
    work_priority = WorkItemPriority_Low;
  } else if (priority == ThreadPriority::REALTIME_AUDIO) {
    work_priority = WorkItemPriority_High;
  }
  return work_priority;
}

static bool InternalStartThread(
    ComPtr<ABI::Windows::System::Threading::IWorkItemHandler> &&handler,
    ABI::Windows::System::Threading::WorkItemPriority work_priority) {

  ComPtr<IThreadPoolStatics> thread_pool;
  HRESULT hr = GetActivationFactory(
      HStringReference(RuntimeClass_Windows_System_Threading_ThreadPool).Get(),
      &thread_pool);
  if (hr != S_OK)
    return false;

  ComPtr<IAsyncAction> action;
  hr = thread_pool->RunWithPriorityAndOptionsAsync(
      handler.Get(),
      work_priority,
      WorkItemOptions_TimeSliced,
      &action);

  return hr == S_OK;
}

bool CreateThreadInternal(PlatformThread::Delegate *delegate,
                          PlatformThreadHandle *out_thread_handle,
                          ThreadPriority priority) {
  HANDLE handle = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);

  HANDLE platform_handle = INVALID_HANDLE_VALUE;

  if (!DuplicateHandle(GetCurrentProcess(),
                       handle,
                       GetCurrentProcess(),
                       &platform_handle,
                       0,
                       false,
                       DUPLICATE_SAME_ACCESS)) {
    CloseHandle(handle);
    return false;
  }

  *out_thread_handle = handle;

  Microsoft::WRL::ComPtr<ABI::Windows::System::Threading::IWorkItemHandler> work_item =
      Microsoft::WRL::Callback<Microsoft::WRL::Implements<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                                                          ABI::Windows::System::Threading::IWorkItemHandler,
                                                          Microsoft::WRL::FtmBase>>
          ([delegate, platform_handle](ABI::Windows::Foundation::IAsyncAction *) {
            __try{
                delegate->ThreadMain();
            }
            __finally{
                SetEvent(platform_handle);
                CloseHandle(platform_handle);
            }
            return S_OK;
          });

  bool result = InternalStartThread(std::move(work_item),
                                    GetWorkItemPriority(priority));
  if (!result) {
    CloseHandle(handle);
    CloseHandle(platform_handle);
  }
  return result;
}

}  // namespace

// static
void PlatformThread::Sleep(uint32_t milliseconds) {
  static HANDLE g_singleton_event = nullptr;

  HANDLE sleep_event = g_singleton_event;

  if (!sleep_event) {
    sleep_event = CreateEventEx(nullptr, nullptr, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);

    if (!sleep_event)
      return;

    HANDLE previous_event = InterlockedCompareExchangePointerRelease(&g_singleton_event, sleep_event, nullptr);

    if (previous_event) {
      CloseHandle(sleep_event);
      sleep_event = g_singleton_event;
    }
  }
  WaitForSingleObjectEx(sleep_event,
                        static_cast<DWORD>(milliseconds),
                        false);
}

// static
bool PlatformThread::CreateWithPriority(Delegate * delegate,
                                        PlatformThreadHandle * thread_handle,
                                        ThreadPriority
priority) {
return
CreateThreadInternal(delegate, thread_handle, priority
);
}

// static
void PlatformThread::Join(PlatformThreadHandle thread_handle) {
  DWORD result = WaitForSingleObjectEx(thread_handle, INFINITE, false);
  if (result != WAIT_OBJECT_0) {
    DWORD error = GetLastError();
  }
  CloseHandle(thread_handle);
}
}  // namespace base