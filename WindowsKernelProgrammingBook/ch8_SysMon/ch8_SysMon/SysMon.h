#pragma once
#include "FastMutex.h"

template<typename T>
struct FullItem {
	LIST_ENTRY Entry;
	T Data;
};

// ��������ȫ��״̬
struct Globals {
	LIST_ENTRY ItemsHead;	// ��������ͷ��
	int ItemCount;			// ����
	FastMutex Mutex;		// ���ٻ�����
};

EXTERN_C void PushItem(PLIST_ENTRY entry);
