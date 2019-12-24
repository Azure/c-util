# platform


## Overview

This document specifies the **platform** adapter for the Azure IoT C SDK. The purpose of _platform_ is
to provide any global init and de-init that may be required, such as `WSAStartup` and `WSACleanup`
for Windows. It also provides the SDK with the proper TLSIO adapter via `platform_get_default_tlsio`.

Although the platform adapter provides a mechanism for performing global init and de-init, device
implementers
may find it makes more sense to perform these operations outside of the scope of the Azure IoT SDK.
In that case, the `platform_init` and `platform_deinit` calls may be left empty.

###   Exposed API
The platform adapter must implement `platform_init` and `platform_deinit`.

###   platform_init

The `platform_init` call performs any global initialization necessary for a particular platform.

```c
int platform_init();
```

**SRS_PLATFORM_30_000: [** The `platform_init` call shall perform any global initialization needed by the platform and return 0 on success. **]**

**SRS_PLATFORM_30_001: [** On failure, `platform_init` shall return a non-zero value. **]**


###   platform_deinit

The `platform_deinit` call performs any global initialization necessary for a particular platform.

```c
void platform_deinit();
```

**SRS_PLATFORM_30_010: [** The `platform_deinit` call shall perform any global deinitialization needed by the platform. **]**
