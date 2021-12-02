#pragma once

enum class ItemType :short {
	None,
	ProcessCreate,
	ProcessExit,
	ThreadCreate,
	ThreadExit,
	ImageLoad
};

// ��¼�¼�������Ϣ
struct ItemHeader {
	ItemType Type;		// �¼�����
	USHORT Size;		// �¼���С(�û��ܿ����Ĵ�С)
	LARGE_INTEGER Time;	// �¼�ʱ��
};

//------�û�ģʽ�ܿ�����������Ϣ--------
// �˳������¼���Ϣ
struct ProcessExitInfo :ItemHeader {
	ULONG ProcessId;
};

// ���������¼���Ϣ
struct ProcessCreateInfo :ItemHeader {
	ULONG ProcessId;			// ����ID
	ULONG ParentProcessId;		// ������ID
	USHORT CommandLineLength;	// �����г���
	USHORT CommandLineOffset;	// ����������ƫ����
	USHORT ImageFileNameLength;	// ӳ��������
	USHORT ImageFileNameOffset;	// ӳ��������ƫ����
};

// �����˳��߳��¼���Ϣ
struct ThreadCreateExitInfo :ItemHeader {
	ULONG ThreadId;
	ULONG ProcessId;
	USHORT ProcessImageFileNameLength;	// ӳ��������(uchar)
	USHORT ProcessImageFileNameOffset;	// ӳ��������ƫ����(uchar)
};

// ӳ������¼���Ϣ
struct ImageLoadInfo :ItemHeader {
	ULONG ProcessId;
	ULONG ImageBase;
	USHORT ImageLoadPathLength;
	USHORT ImageLoadPathOffset;
};