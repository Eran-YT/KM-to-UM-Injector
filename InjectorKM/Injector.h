#pragma once
#define WARNINGS 4514 4365 4668 5026 5027

#pragma warning(disable: WARNINGS)
#include <ntifs.h>
#include <ntddk.h>
#pragma warning(disable: WARNINGS)

#define DRIVER_PREFIX "Injector: "

NTSTATUS inject_to_process(ULONG pid, PVOID buffer, SIZE_T size);
NTSTATUS create_thread_at_address(PVOID address);
PVOID NTAPI get_kernel_proc_address(LPCWSTR SystemRoutineName);


