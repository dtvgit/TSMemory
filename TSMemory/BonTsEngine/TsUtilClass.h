// TsUtilClass.h: TS���[�e�B���e�B�[�N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


/////////////////////////////////////////////////////////////////////////////
// �_�C�i�~�b�N���t�@�����X�Ǘ��x�[�X�N���X
/////////////////////////////////////////////////////////////////////////////

class CDynamicReferenceable
{
public:
	CDynamicReferenceable();
	virtual ~CDynamicReferenceable();

	void AddRef(void);
	void ReleaseRef(void);
	DWORD GetRefCount(void) const;

private:
	DWORD m_dwRefCount;
};


/////////////////////////////////////////////////////////////////////////////
// �N���e�B�J���Z�N�V�������b�p�[�N���X
/////////////////////////////////////////////////////////////////////////////

class CCriticalLock
{
public:
	CCriticalLock();
	virtual ~CCriticalLock();

	void Lock(void);
	void Unlock(void);
	bool TryLock(DWORD TimeOut=0);
private:
	CRITICAL_SECTION m_CriticalSection;
};


/////////////////////////////////////////////////////////////////////////////
// �u���b�N�X�R�[�v���b�N�N���X
/////////////////////////////////////////////////////////////////////////////

class CBlockLock
{
public:
	CBlockLock(CCriticalLock *pCriticalLock);
	virtual ~CBlockLock();
		
private:
	CCriticalLock *m_pCriticalLock;
};

class CTryBlockLock
{
public:
	CTryBlockLock(CCriticalLock *pCriticalLock);
	bool TryLock(DWORD TimeOut=0);
	~CTryBlockLock();
private:
	CCriticalLock *m_pCriticalLock;
	bool m_bLocked;
};


/////////////////////////////////////////////////////////////////////////////
// �C�x���g�N���X
/////////////////////////////////////////////////////////////////////////////

class CLocalEvent
{
	HANDLE m_hEvent;
public:
	CLocalEvent();
	~CLocalEvent();
	bool Create(bool bManual = false, bool bInitialState = false);
	bool IsCreated() const;
	void Close();
	bool Set();
	bool Reset();
	DWORD Wait(DWORD Timeout = INFINITE);
	bool IsSignaled();
};


/////////////////////////////////////////////////////////////////////////////
// �g���[�X�N���X
/////////////////////////////////////////////////////////////////////////////

class CTracer
{
	TCHAR m_szBuffer[256];
public:
	virtual ~CTracer() {}
	void Trace(LPCTSTR pszOutput, ...);
	void TraceV(LPCTSTR pszOutput,va_list Args);
protected:
	virtual void OnTrace(LPCTSTR pszOutput)=0;
};


/////////////////////////////////////////////////////////////////////////////
// CRC�v�Z�N���X
/////////////////////////////////////////////////////////////////////////////

class CCrcCalculator
{
public:
	static WORD CalcCrc16(const BYTE *pData, DWORD DataSize, WORD wCurCrc = 0xFFFF);
	static DWORD CalcCrc32(const BYTE *pData, DWORD DataSize, DWORD dwCurCrc = 0xFFFFFFFFUL);
};
