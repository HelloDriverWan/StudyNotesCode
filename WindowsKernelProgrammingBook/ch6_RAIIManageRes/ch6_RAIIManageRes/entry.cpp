#include <ntifs.h>

struct FastMutex {
	void init() {
		ExInitializeFastMutex(&_FastMutex);
	}
	void Lock() {
		ExAcquireFastMutex(&_FastMutex);
	}
	void UnLock() {
		ExReleaseFastMutex(&_FastMutex);
	}
private:
	FAST_MUTEX _FastMutex;
};

struct Mutex {
	void init() {
		// ������øú���һ������ʼ��������
		KeInitializeMutex(&_Mutex, 0);
	}
	void Lock() {
		// �ȴ�������
		KeWaitForSingleObject(&_Mutex, Executive, KernelMode, FALSE, nullptr);
	}
	void UnLock() {
		// �ͷŻ�����
		KeReleaseMutex(&_Mutex, FALSE);
	}
private:
	KMUTEX _Mutex;
};

template<typename TLock>
struct AutoLock{
	AutoLock(TLock& lock) :_lock(lock) {
		lock.Lock();
	}
	~AutoLock() {
		_lock.UnLock();
	}
private:
	TLock& _lock;
};

Mutex myMutex;
void init() {
	myMutex.init();
}
void DoWork() {
	AutoLock<Mutex> locker(myMutex);
	// ִ����������
}

ERESOURCE resource;
void WriteData() {
	KeEnterCriticalRegion();							// ����ؼ���(����APC)
	ExAcquireResourceExclusiveLite(&resource, TRUE);	// ��ȡִ������Դ������(д)
	// ExAcquireResourceSharedLite(&resource, TRUE);	// ��ȡִ������Դ������(��)
	// ExEnterCriticalRegionAndAcquireResourceExclusive(&resource);	// ����ؼ���+��ȡ������
	// ExEnterCriticalRegionAndAcquireResourceShared(&resource);	// ����ؼ���+��ȡ������
	// do something
	// ExReleaseResourceAndLeaveCriticalRegion(&resource);			// �ͷ���+�˳��ؼ���
	ExReleaseResourceLite(&resource);					// �ͷ���
	KeLeaveCriticalRegion();	//	�˳��ؼ���
}

VOID DriverUnload(PDRIVER_OBJECT pDriverObject) {

	DbgPrint("Bye World!\n");
}

NTSTATUS DriverEntry(PDRIVER_OBJECT pDriverObject, PUNICODE_STRING pReg) {
	pDriverObject->DriverUnload = DriverUnload;
	DbgPrint("Hello World!\n");
	return STATUS_SUCCESS;
}