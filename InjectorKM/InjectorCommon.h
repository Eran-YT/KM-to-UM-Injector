#pragma once
#define INJECTOR_NAME L"Injector"

#define IOCTL_INJECT_BY_PID	CTL_CODE(0x8000, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

#ifndef BYTE
#define BYTE unsigned char
#endif

struct InjectorInfo
{
	ULONG pid;
	ULONG buffer_size;
	BYTE buffer[1 << 16];
};
