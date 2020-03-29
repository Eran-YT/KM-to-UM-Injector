#include "Injector.h"
#include "InjectorCommon.h"


// PROTOTYPES

DRIVER_UNLOAD InjectorUnload;
DRIVER_DISPATCH InjectorCreateClose, InjectorDeviceControl;



extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING) {
	KdPrint((DRIVER_PREFIX "DriverEntry entered\n"));

	auto status = STATUS_SUCCESS;
	UNICODE_STRING deviceName = RTL_CONSTANT_STRING(L"\\Device\\" INJECTOR_NAME);
	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\" INJECTOR_NAME);
	PDEVICE_OBJECT DeviceObject = nullptr;

	do {
		status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create device object (status=%08X)\n", status));
			break;
		}

		status = IoCreateSymbolicLink(&symName, &deviceName);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create symbolic link (status=%08X)\n", status));
			break;
		}
	} while (false);

	if (!NT_SUCCESS(status)) {
		if (DeviceObject)
			IoDeleteDevice(DeviceObject);
	}

	DriverObject->DriverUnload = InjectorUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = InjectorCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = InjectorDeviceControl;

	KdPrint((DRIVER_PREFIX "DriverEntry completed successfully\n"));

	return status;
}

void InjectorUnload(PDRIVER_OBJECT DriverObject) {
	UNICODE_STRING symName = RTL_CONSTANT_STRING(L"\\??\\" INJECTOR_NAME);
	IoDeleteSymbolicLink(&symName);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS InjectorCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return STATUS_SUCCESS;
}

NTSTATUS InjectorDeviceControl(PDEVICE_OBJECT, PIRP Irp) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;
	auto len = 0;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_INJECT_BY_PID: {
		auto size = stack->Parameters.DeviceIoControl.InputBufferLength;
		if (size % sizeof(ULONG) != 0) {
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		if (size < sizeof InjectorInfo) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		InjectorInfo* data = (InjectorInfo*)Irp->AssociatedIrp.SystemBuffer;
		if (data == nullptr) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		if (data->buffer_size > (size - (sizeof ULONG) * 2)) {
			status = STATUS_INVALID_BUFFER_SIZE;
			break;
		}

		status = inject_to_process(data->pid, &data->buffer, data->buffer_size);
		break;
	}

	default:
		status = STATUS_INVALID_DEVICE_REQUEST;
		break;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = len;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS inject_to_process(ULONG pid, PVOID buffer, SIZE_T size)
{
	UNREFERENCED_PARAMETER(buffer);
	UNREFERENCED_PARAMETER(size);

	KdPrint((DRIVER_PREFIX "inject_to_process entered\n"));

	HANDLE handle = UlongToHandle(pid);
	PEPROCESS process_ptr = nullptr;

	NTSTATUS status = PsLookupProcessByProcessId(handle, &process_ptr);
	if (!NT_SUCCESS(status)) {
		KdPrint((DRIVER_PREFIX "PsLookupProcessByProcessId failed (status=%08X)\n", status));
		return status;
	}

	KAPC_STATE apc;
	KeStackAttachProcess(process_ptr, &apc);

	PVOID address = nullptr;
	SIZE_T alloc_size = size;
	status = ZwAllocateVirtualMemory(ZwCurrentProcess(), &address, 0, &alloc_size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (NT_SUCCESS(status) && address != nullptr) {
		KdPrint((DRIVER_PREFIX "Allocated memory in process\n"));
		RtlCopyMemory(address, buffer, size);

		status = create_thread_at_address(address);

	}
	else {
		KdPrint((DRIVER_PREFIX "Failed to allocate memory (status=%08X)\n", status));
	}
	KeUnstackDetachProcess(&apc);

	if (process_ptr) {
		ObDereferenceObject(process_ptr);
	}

	return STATUS_SUCCESS;
}

PVOID NTAPI get_kernel_proc_address(LPCWSTR SystemRoutineName)
{
	UNICODE_STRING Name;
	RtlInitUnicodeString(&Name, SystemRoutineName);
	return MmGetSystemRoutineAddress(&Name);
}

NTSTATUS create_thread_at_address(PVOID address)
{
	KdPrint((DRIVER_PREFIX "create_thread_at_address entered\n"));

	using _RtlCreateUserThread = NTSTATUS(NTAPI*)(
		IN HANDLE               ProcessHandle,
		IN PSECURITY_DESCRIPTOR SecurityDescriptor,
		IN BOOLEAN              CreateSuspended,
		IN ULONG                StackZeroBits,
		IN OUT PULONG           StackReserved,
		IN OUT PULONG           StackCommit,
		IN PVOID                StartAddress,
		IN PVOID                StartParameter,
		OUT PHANDLE             ThreadHandle,
		OUT PCLIENT_ID          ClientID
		);
	auto _CreateUserThread = static_cast<_RtlCreateUserThread>(get_kernel_proc_address(L"RtlCreateUserThread"));
	if (_CreateUserThread == nullptr) {
		return STATUS_NOT_IMPLEMENTED;
	}
	HANDLE thrd_handle;
	CLIENT_ID client_id;

	NTSTATUS status = _CreateUserThread(
		ZwCurrentProcess(),
		NULL,
		FALSE, 
		0,
		NULL,
		NULL,
		address,
		nullptr,
		&thrd_handle,
		&client_id
		);
	KdPrint((DRIVER_PREFIX "Created thread with status %08X\n", status));

	return status;
}