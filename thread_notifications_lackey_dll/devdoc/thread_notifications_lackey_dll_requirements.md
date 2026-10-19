# `thread_notifications_lackey_dll` requirements

## Overview

`thread_notifications_lackey_dll` is module that implements `DllMain` in order to hook thread attach/detach notifications.

It is named "lackey" because that is what our friend Raymond called it :-).

Its goal is to abstract the implementation of a Dll in order to be able to hook DLL thread attach/detach notifications (`DllMain` is not accesible in executables).

Only one callback with no context is supported. If multiple consumers of callbacks are needed, that functionality would be the responsibility of a separate unit that could demux the callbacks and have contexts stored.

## References

[https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain](https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain)

[Fibers aren’t useful for much any more; there’s just one corner of it that remains useful for a reason unrelated to](https://devblogs.microsoft.com/oldnewthing/20191011-00/?p=102989)

## Exposed API

```c
#define THREAD_NOTIFICATIONS_LACKEY_DLL_REASON_VALUES \
    THREAD_NOTIFICATIONS_LACKEY_DLL_REASON_THREAD_ATTACH, \
    THREAD_NOTIFICATIONS_LACKEY_DLL_REASON_THREAD_DETACH

MU_DEFINE_ENUM(THREAD_NOTIFICATIONS_LACKEY_DLL_REASON, THREAD_NOTIFICATIONS_LACKEY_DLL_REASON_VALUES)

typedef void (*THREAD_NOTIFICATIONS_LACKEY_DLL_CALLBACK_FUNC)(THREAD_NOTIFICATIONS_LACKEY_DLL_REASON reason);

MOCKABLE_FUNCTION(, int, thread_notifications_lackey_dll_init_callback, THREAD_NOTIFICATIONS_LACKEY_DLL_CALLBACK_FUNC, thread_notification_cb);
MOCKABLE_FUNCTION(, int, thread_notifications_lackey_dll_deinit_callback);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
```

### thread_notifications_lackey_dll_init_callback

```c
MOCKABLE_FUNCTION(, int, thread_notifications_lackey_dll_init_callback, THREAD_NOTIFICATIONS_LACKEY_DLL_CALLBACK_FUNC, thread_notification_cb, void*, context);
```

`thread_notifications_lackey_set_callback` initializes the callback that is to be called whenever a thread attach/detach event happens.

**SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_001: [** If `thread_notification_cb` is `NULL`, `thread_notifications_lackey_set_callback` shall fail and return a non-zero value. **]**

**SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_003: [** If the callback was already initialized, `thread_notifications_lackey_dll_init_callback` shall fail and return a non-zero value. **]**

**SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_002: [** Otherwise, `thread_notifications_lackey_dll_init_callback` shall initialize the callback maintained by the module with `thread_notification_cb`, succeed and return 0. **]**

### thread_notifications_lackey_dll_deinit_callback

```c
MOCKABLE_FUNCTION(, int, thread_notifications_lackey_dll_deinit_callback);
```

`thread_notifications_lackey_dll_deinit_callback` clears the thread notifications callback.

**SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_004: [** `thread_notifications_lackey_dll_deinit_callback` shall set the callback maintained by the module to `NULL`. **]**

### DllMain

```c
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved);
```

`DllMain` implements the DLL main entry point.

**SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_005: [** If `fdwReason` is `DLL_PROCESS_ATTACH`: **]**

- **SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_007: [** `DllMain` shall return `TRUE`. **]**

**SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_009: [** If `fdwReason` is `DLL_PROCESS_DETACH`: **]**

- **SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_010: [** `DllMain` shall return `TRUE`. **]**

#### DllMain design notes

`DllMain` does no initialization or cleanup for process attach/detach. In particular it does not use logging, because the
module keeps no state that needs it and because of the following constraints from the
[`DllMain` documentation](https://learn.microsoft.com/en-us/windows/win32/dlls/dllmain) and
[DLL best practices](https://learn.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-best-practices):

- `DllMain` runs under the loader lock and must not call `LoadLibrary` directly or indirectly. `logger_init` calls `SymInitialize`, which makes dbghelp load `symsrv.dll` when it is found next to `dbghelp.dll` (this is the case on the ARM64 build agents).
- During process termination other DLLs may already have been detached. `symsrv.dll` is loaded after this module, so it is detached before this module, and `logger_deinit` → `SymCleanup` would call into an unloaded `symsrv.dll`. The process dies before stdout is flushed and the executable's output is lost although the exit code is 0.
- The ideal `DLL_PROCESS_DETACH` handler is empty: when the process terminates the operating system reclaims resources.

Failures of `thread_notifications_lackey_dll_init_callback` are reported to the caller through its return value, which the caller logs using its own (properly initialized) logger.

**SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_011: [** If `fdwReason` is `DLL_THREAD_ATTACH`: **]**

- **SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_013: [** If a callback was passed by the user through a `thread_notifications_lackey_dll_init_callback` call, the callback shall be called with `THREAD_NOTIFICATIONS_LACKEY_DLL_REASON_THREAD_ATTACH`. **]**

- **SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_012: [** `DllMain` shall return `TRUE`. **]**

**SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_014: [** If `fdwReason` is `DLL_THREAD_DETACH`: **]**

- **SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_016: [** If a callback was passed by the user through a `thread_notifications_lackey_dll_init_callback` call, the callback shall be called with `THREAD_NOTIFICATIONS_LACKEY_DLL_REASON_THREAD_DETACH`. **]**

- **SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_015: [** `DllMain` shall return `TRUE`. **]**

**SRS_THREAD_NOTIFICATIONS_LACKEY_DLL_01_017: [** If `fdwReason` is any other value, `DllMain` shall terminate the process. **]**
