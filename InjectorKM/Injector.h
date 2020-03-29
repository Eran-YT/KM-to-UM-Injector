#pragma once
#define WARNINGS 4514 4365 5026 5027

#pragma warning(disable: WARNINGS)
#include <ntddk.h>
#pragma warning(disable: WARNINGS)

#define INJECTOR_NAME L"Injector"
#define DRIVER_PREFIX "Injector: "


#define IOCTL_INJECT_BY_PID	CTL_CODE(0x8000, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)


