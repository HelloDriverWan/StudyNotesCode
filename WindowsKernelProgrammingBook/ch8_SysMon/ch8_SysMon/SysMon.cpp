#include "pch.h"
#include "SysMon.h"
#include "SysMonCommon.h"
#include "AutoLock.h"

#define DEVICE_NAME L"\\Device\\SysMon"
#define SYMBOL_LINK_NAME L"\\??\\SysMon"
#define DRIVER_PREFIX "SysMon: "
#define DRIVER_TAG 'lsmn'
Globals g_Globals;

EXTERN_C
NTSYSAPI
UCHAR*
PsGetProcessImageFileName(
	__in PEPROCESS Process
);

EXTERN_C
VOID
OnProcessNotify(
	_Inout_ PEPROCESS Process,
	_In_ HANDLE ProcessId,
	_Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo
	) {
	if (CreateInfo) {	// ���̴���ʱ
		// ��ȡ���ݽṹ��С
		USHORT allocSize = sizeof(FullItem<ProcessCreateInfo>);
		// ��ȡ������+���ݽṹ��С
		USHORT CommandLineSize = 0;
		if (CreateInfo->CommandLine) {
			CommandLineSize = CreateInfo->CommandLine->Length +2 ;
			allocSize += CommandLineSize;
		}
		// ��ȡӳ������Ϣ�ӵ������ڴ��С��
		USHORT ImageFileNameSize = 0;
		if (CreateInfo->ImageFileName) {
			ImageFileNameSize = CreateInfo->ImageFileName->Length + 2;
			allocSize += ImageFileNameSize;
		}

		// �����ڴ�
		auto info = (FullItem<ProcessCreateInfo>*)ExAllocatePoolWithTag(PagedPool, allocSize, DRIVER_TAG);
		if (info == nullptr) {
			KdPrint((DRIVER_PREFIX "failed allocation!\r\n"));
			return;
		}

		// ���̶�����
		auto& item = info->Data;
		KeQuerySystemTimePrecise(&item.Time);
		ExSystemTimeToLocalTime(&item.Time, &item.Time);
		item.Type = ItemType::ProcessCreate;
		item.Size = sizeof(FullItem<ProcessCreateInfo>) + CommandLineSize + ImageFileNameSize;
		item.ProcessId = HandleToULong(ProcessId);
		item.ParentProcessId = HandleToULong(CreateInfo->ParentProcessId);
		
		// ��������������Ϣ
		if (CommandLineSize > 0) {
			// ����������У��͸��Ƶ��ṹ�������������Ϣ
			::memcpy((UCHAR*)&item + sizeof(item), CreateInfo->CommandLine->Buffer, CommandLineSize);
			item.CommandLineLength = CommandLineSize / sizeof(WCHAR);
			item.CommandLineOffset = sizeof(item);
		}
		else {
			// ���û�����У��͸�ֵ0
			item.CommandLineLength = 0;
			item.CommandLineOffset = 0;
		}

		// ���ӳ���������Ϣ
		if (ImageFileNameSize > 0) {
			::memcpy((UCHAR*)&item + sizeof(item) + CommandLineSize, CreateInfo->ImageFileName->Buffer,ImageFileNameSize);
			item.ImageFileNameLength = ImageFileNameSize / sizeof(WCHAR);
			item.ImageFileNameOffset = sizeof(item) + CommandLineSize;
		}
		else {
			item.ImageFileNameLength = 0;
			item.ImageFileNameOffset = 0;
		}

		// ���뵽������
		PushItem(&info->Entry);

	}
	else {	// �����˳�ʱ
		// ����ռ�
		auto info = (FullItem<ProcessExitInfo>*)ExAllocatePoolWithTag(PagedPool, sizeof(FullItem<ProcessExitInfo>),DRIVER_TAG);
		if (!info) {
			KdPrint((DRIVER_PREFIX "failed allocation!\r\n"));
			return;
		}
		// �������
		auto& item = info->Data;
		KeQuerySystemTimePrecise(&item.Time);
		ExSystemTimeToLocalTime(&item.Time, &item.Time);
		item.Type = ItemType::ProcessExit;
		item.ProcessId = HandleToULong(ProcessId);
		item.Size = sizeof(ProcessExitInfo);

		// ���뵽������
		PushItem(&info->Entry);
	}
}

EXTERN_C
VOID
OnThreadNotify(
	_In_ HANDLE ProcessId,
	_In_ HANDLE ThreadId,
	_In_ BOOLEAN Create
) {
	auto size = sizeof(FullItem<ThreadCreateExitInfo>);

	PEPROCESS pEprocess = NULL;
	PUCHAR ProcessImageFileName = NULL;
	auto status = PsLookupProcessByProcessId(ProcessId,&pEprocess);
	auto ProcessImageFileNameLength = 0;
	if (NT_SUCCESS(status)) {
		ProcessImageFileName = PsGetProcessImageFileName(pEprocess);
		if (ProcessImageFileName) {
			ProcessImageFileNameLength = strlen((const char*)ProcessImageFileName) +1;
			size += ProcessImageFileNameLength;
		}
	}
	

	auto info = (FullItem<ThreadCreateExitInfo>*)ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);
	if (!info) {
		KdPrint((DRIVER_PREFIX "failed allocation!\r\n"));
		return;
	}
	auto& item = info->Data;
	KeQuerySystemTimePrecise(&item.Time);
	ExSystemTimeToLocalTime(&item.Time, &item.Time);
	item.Size = size;
	item.Type = Create ? ItemType::ThreadCreate : ItemType::ThreadExit;
	item.ProcessId = HandleToULong(ProcessId);
	item.ThreadId = HandleToULong(ThreadId);

	if (ProcessImageFileNameLength > 0) {
		::memcpy((UCHAR*)&item + sizeof(item),ProcessImageFileName,ProcessImageFileNameLength);
		item.ProcessImageFileNameLength = ProcessImageFileNameLength;
		item.ProcessImageFileNameOffset = sizeof(item);
	}
	else {
		item.ProcessImageFileNameLength = 0;
		item.ProcessImageFileNameOffset = 0;
	}

	PushItem(&info->Entry);
}

EXTERN_C
VOID
OnImageNotify(
	_In_opt_ PUNICODE_STRING FullImageName,
	_In_ HANDLE ProcessId,                // pid into which image is being mapped
	_In_ PIMAGE_INFO ImageInfo
) {
	auto size = sizeof(FullItem<ImageLoadInfo>);
	auto ImageNameSize = 0;
	if (FullImageName) {
		ImageNameSize = FullImageName->Length + 2;
		size += ImageNameSize;
	}


	auto info = (FullItem<ImageLoadInfo>*)ExAllocatePoolWithTag(PagedPool, size, DRIVER_TAG);
	if (!info) {
		KdPrint((DRIVER_PREFIX "failed allocation!\r\n"));
		return;
	}
	auto& item = info->Data;
	KeQuerySystemTimePrecise(&item.Time);
	ExSystemTimeToLocalTime(&item.Time, &item.Time);
	item.Size = size;
	item.Type = ItemType::ImageLoad;
	item.ProcessId = HandleToULong(ProcessId);
	item.ImageBase = (ULONG)(ImageInfo->ImageBase);


	if (ImageNameSize > 0) {
		::memcpy((UCHAR*)&item + sizeof(item), FullImageName->Buffer, ImageNameSize);
		item.ImageLoadPathLength = ImageNameSize;
		item.ImageLoadPathOffset = sizeof(item);
	}
	else {
		item.ImageLoadPathLength = 0;
		item.ImageLoadPathOffset = 0;
	}

	PushItem(&info->Entry);
}

EXTERN_C
NTSTATUS
SysMonRead(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
) {
	auto stack = IoGetCurrentIrpStackLocation(Irp);
	auto len = stack->Parameters.Read.Length;
	auto status = STATUS_SUCCESS;
	auto count = 0;
	
	NT_ASSERT(Irp->MdlAddress);	// ����ʹ�õ���ֱ��I/O

	// �õ�������
	auto buffer = (PUCHAR)MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
	if (!buffer) {
		status = STATUS_INSUFFICIENT_RESOURCES;
	}
	else {
		AutoLock<FastMutex> lock(g_Globals.Mutex);// Ҫ��û�õ���������զ����
		while (true) {
			// ������˾�����
			if (IsListEmpty(&g_Globals.ItemsHead))
				break;
			// ������������һ��
			auto entry = RemoveHeadList(&g_Globals.ItemsHead);
			auto info = CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry);	// Ϊɶ������ṹ
			auto size = info->Data.Size;
			if (len < size) {
				// ���ˣ��ٲ��ȥ�������ˣ�����
				InsertHeadList(&g_Globals.ItemsHead, entry);
				break;
			}
			g_Globals.ItemCount--;
			// �����ߵ���һ��Ƶ�������
			::memcpy(buffer, &info->Data, size);
			// ����ʣ��ռ�
			len -= size;
			// �����������ƶ�
			buffer += size;
			// �������ߵ�����
			count +=size;

			ExFreePool(info);
		}		
	}
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = count;
	IoCompleteRequest(Irp, 0);
	return status;


}

EXTERN_C NTSTATUS
ZeroCreateClose(PDEVICE_OBJECT, PIRP Irp) {
	auto status = STATUS_SUCCESS;
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, 0);
	return status;
}

EXTERN_C
VOID
DriverUnload(
	_In_ struct _DRIVER_OBJECT* pDriverObject
) {
	PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
	PsRemoveCreateThreadNotifyRoutine(OnThreadNotify);
	PsRemoveLoadImageNotifyRoutine(OnImageNotify);

	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYMBOL_LINK_NAME);
	IoDeleteSymbolicLink(&symLinkName);
	IoDeleteDevice(pDriverObject->DeviceObject);

	while (!IsListEmpty(&g_Globals.ItemsHead)) {
		auto entry = RemoveHeadList(&g_Globals.ItemsHead);
		ExFreePool(CONTAINING_RECORD(entry, FullItem<ItemHeader>, Entry));
	}
}

EXTERN_C
NTSTATUS
DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pReg) {

	// ��ʼ������&��ʼ�����ٻ�����
	InitializeListHead(&g_Globals.ItemsHead);
	g_Globals.Mutex.init();

	PDEVICE_OBJECT pDeviceObject = NULL;
	UNICODE_STRING symLinkName = RTL_CONSTANT_STRING(SYMBOL_LINK_NAME);
	bool isSymbolLinkCreate = false;
	auto status = STATUS_SUCCESS;

	do {
		// �����豸����
		UNICODE_STRING devName = RTL_CONSTANT_STRING(DEVICE_NAME);
		status = IoCreateDevice(pDriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &pDeviceObject);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create symbolic link (0x%08X)\n", status));
			break;
		}
		// ����ֱ��IO
		pDeviceObject->Flags |= DO_DIRECT_IO;

		// ������������
		status = IoCreateSymbolicLink(&symLinkName, &devName);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to create symbolic link (0x%08X)\n", status));
			break;
		}
		isSymbolLinkCreate = true;

		// ע�����֪ͨ
		status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register process callback (0x%08X)\n", status));
			break;
		}

		// ע���߳�֪ͨ
		status = PsSetCreateThreadNotifyRoutine(OnThreadNotify);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register thread callback (0x%08X)\n", status));
			break;
		}

		// ע��ӳ��֪ͨ
		status = PsSetLoadImageNotifyRoutine(OnImageNotify);
		if (!NT_SUCCESS(status)) {
			KdPrint((DRIVER_PREFIX "failed to register image callback (0x%08X)\n", status));
			break;
		}
	} while (false);
	// ���ʧ����,�ͻع�����
	if (!NT_SUCCESS(status)) {
		if (pDeviceObject) IoDeleteDevice(pDeviceObject);
		if (isSymbolLinkCreate) IoDeleteSymbolicLink(&symLinkName);
	}

	// ���÷ַ�����
	pDriverObject->DriverUnload = DriverUnload;
	pDriverObject->MajorFunction[IRP_MJ_CREATE] = pDriverObject->MajorFunction[IRP_MJ_CLOSE] = ZeroCreateClose;
	pDriverObject->MajorFunction[IRP_MJ_READ] = SysMonRead;

	return status;
}


void PushItem(PLIST_ENTRY entry) {
	// �õ�������,���������Զ��ͷ�
	AutoLock<FastMutex> lock(g_Globals.Mutex);
	
	// ����ģ�����
	// �������������,��ÿ����һ����ɾ��һ��������
	if (g_Globals.ItemCount > 1024) {
		// �Ƴ���һ����Ա
		auto head = RemoveHeadList(&g_Globals.ItemsHead);
		g_Globals.ItemCount--;
		// ��ȡ�ó�Ա�׵�ַ,�����ڴ��ͷ�
		auto item = CONTAINING_RECORD(head, FullItem<ItemHeader>, Entry);
		ExFreePool(item);
	}
	// �������ݵ�����β��
	InsertTailList(&g_Globals.ItemsHead, entry);
	g_Globals.ItemCount++;
}

