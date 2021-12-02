#include <ntifs.h>
#include <ntddk.h>
#include "PBCommon.h"
#define DEVICE_NAME L"\\Device\\PriorityBooster"
#define SYMBLE_LINK_NAME L"\\??\\PriorityVooster"

_Use_decl_annotations_
NTSTATUS
PriorityBoosterCreateClose(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	Irp->IoStatus.Status = STATUS_SUCCESS;	// ָ��ʲô״̬��ɸ�����
	Irp->IoStatus.Information = 0;			// ��ͬ�����в�ͬ����,����������Ҫ����Ϊ0
	IoCompleteRequest(Irp,IO_NO_INCREMENT);	// ���IRP��Ӧ,���IRP���ظ�������(IO������),Ȼ�������֪ͨ�ͻ��������������
	return STATUS_SUCCESS;
}

_Use_decl_annotations_
NTSTATUS
PriorityBoosterDeviceControl(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto status = STATUS_SUCCESS;

	switch (stack->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_PRIORITY_BOOSTER_SET_PRIORITY: {
		// do the work
		// �жϽ��յ��Ļ�������С�Ƿ��㹻
		if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(ThreadData)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		// ��ȡ����������
		auto data = (PThreadData)stack->Parameters.DeviceIoControl.Type3InputBuffer;
		if (data == nullptr) {
			status = STATUS_INVALID_PARAMETER;
			break;
		}
		__try {
			// �����Ϸ��Լ��
			if (data->Priority < 1 || data->Priority>31) {
				status = STATUS_INVALID_PARAMETER;
				break;
			}
			// ���߳�idת���ɾ��,Ȼ��ͨ��API��ȡ�߳̽ṹ,��API�������߳̽ṹ�����ü���,��Ҫ�ֶ�����
			PETHREAD pEthread;
			status = PsLookupThreadByThreadId(ULongToHandle(data->ThreadId), &pEthread);
			if (!NT_SUCCESS(status)) {
				break;
			}
			// �޸����ȼ�
			KeSetPriorityThread(pEthread, data->Priority);

			// �ֶ��������ü���
			ObDereferenceObject(pEthread);
		}
		__except(EXCEPTION_EXECUTE_HANDLER){	// EXCEPTION_EXECUTE_HANDLER��ʾ�κ��쳣���ᴦ��
			status = STATUS_ACCESS_VIOLATION;
		}
		break;

	}
	default: {
		status = STATUS_INVALID_DEVICE_REQUEST;
		break; 
	}
	}
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;

}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject) {
	// ��ԭ���������Ĳ���
	// ɾ����������
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMBLE_LINK_NAME);
	IoDeleteSymbolicLink(&symLink);
	IoDeleteDevice(pDriverObject->DeviceObject);
	KdPrint(("Bye World!\n"));
}

EXTERN_C NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pReg) {
	// ָ��Unload����
	pDriverObject->DriverUnload = DriverUnload;
	// �����豸����������ҪCREATE��CLOSE�����ַ�����
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = PriorityBoosterCreateClose;
	pDriverObject->MajorFunction[IRP_MJ_CLOSE] = PriorityBoosterCreateClose;
	// ͨ���豸����ʹ����Ϣ�ܴ�R3��������,��Ҫʹ��DeviceIoControl,��API֪ͨ����DEVICE_CONTROL
	// ͨ�����ַ�������ͨ����Ҫ����������뻺����
	pDriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PriorityBoosterDeviceControl;

	// �����豸����,�������������ֻ��Ҫһ���豸����,ͨ����������ָ��������
	UNICODE_STRING devName = RTL_CONSTANT_STRING(DEVICE_NAME);
	PDEVICE_OBJECT pDeviceObject = NULL;
	NTSTATUS ntStatus = IoCreateDevice(pDriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObject);

	// �ṩ��������
	UNICODE_STRING symLink = RTL_CONSTANT_STRING(SYMBLE_LINK_NAME);
	ntStatus = IoCreateSymbolicLink(&symLink, &devName);
	if (!NT_SUCCESS(ntStatus)) {
		// ���ʧ����,��Ҫ����֮ǰ�Ĳ���,��ΪDriverEntry�з����˷ǳɹ���״̬,DriverUnload�ǲ��ᱻ����ִ�е�
		KdPrint(("Failed to create symbolic link (0x%08x)\n",ntStatus));
		IoDeleteDevice(pDeviceObject);
		return ntStatus;
	}


	KdPrint(("Hello World!\n"));
	return STATUS_SUCCESS;
}