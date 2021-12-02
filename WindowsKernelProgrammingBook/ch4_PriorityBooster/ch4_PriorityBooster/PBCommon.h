#pragma once
#include <ntifs.h>
// ����:
// DeviceType: �豸����,������������0x8000��ʼ
// Function: ����ָ����������,���ڵ�����������0x800��ʼ
// Method: ָ���û��ṩ�������������������δ���������
// Access: ָ���ǵ�������������������������
#define CTL_CODE(DeviceType,Function,Method,Access)(\
((DeviceType) << 16) | ((Access)<<14) | ((Function) << 2) | Method)
#define PRIORITY_BOOSTER_DEVICE 0x8000
#define IOCTL_PRIORITY_BOOSTER_SET_PRIORITY CTL_CODE(PRIORITY_BOOSTER_DEVICE,0x800,METHOD_NEITHER,FILE_ANY_ACCESS)
typedef struct _ThreadData {
	ULONG ThreadId;		// ��Ҫ�޸ĵ��̵߳�ID
	int Priority;		// ��Ҫ�޸ĵ����ȼ�
}ThreadData, * PThreadData;



