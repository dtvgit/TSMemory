// TsDescriptor.h: �L�q�q���b�p�[�N���X�̃C���^�[�t�F�C�X
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include <Vector>
#include "MediaData.h"


using std::vector;


// ISO 639 language code
#define LANGUAGE_CODE_JPN	0x6A706EUL	// ���{��
#define LANGUAGE_CODE_ENG	0x656E67UL	// �p��
#define LANGUAGE_CODE_DEU	0x646575UL	// �h�C�c��
#define LANGUAGE_CODE_FRA	0x667261UL	// �t�����X��
#define LANGUAGE_CODE_ITA	0x697461UL	// �C�^���A��
#define LANGUAGE_CODE_RUS	0x727573UL	// ���V�A��
#define LANGUAGE_CODE_ZHO	0x7A686FUL	// ������
#define LANGUAGE_CODE_KOR	0x6B6F72UL	// �؍���
#define LANGUAGE_CODE_SPA	0x737061UL	// �X�y�C����
#define LANGUAGE_CODE_ETC	0x657463UL	// ���̑�


/////////////////////////////////////////////////////////////////////////////
// �L�q�q�̊��N���X
/////////////////////////////////////////////////////////////////////////////

class CBaseDesc
{
public:
	CBaseDesc();
	CBaseDesc(const CBaseDesc &Operand);
	virtual ~CBaseDesc();
	CBaseDesc & operator = (const CBaseDesc &Operand);

	virtual void CopyDesc(const CBaseDesc *pOperand);
	const bool ParseDesc(const BYTE *pHexData, const WORD wDataLength);

	const bool IsValid(void) const;
	const BYTE GetTag(void) const;
	const BYTE GetLength(void) const;

	virtual void Reset(void);

protected:
	virtual const bool StoreContents(const BYTE *pPayload);

	BYTE m_byDescTag;	// �L�q�q�^�O
	BYTE m_byDescLen;	// �L�q�q��
	bool m_bIsValid;	// ��͌���
};


/////////////////////////////////////////////////////////////////////////////
// [0x09] Conditional Access Method �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CCaMethodDesc : public CBaseDesc
{
public:
	enum {DESC_TAG = 0x09U};

	CCaMethodDesc();
	CCaMethodDesc(const CCaMethodDesc &Operand);
	CCaMethodDesc & operator = (const CCaMethodDesc &Operand);

// CBaseDesc
	virtual void CopyDesc(const CBaseDesc *pOperand);
	virtual void Reset(void);

// CCaMethodDesc
	const WORD GetCaMethodID(void) const;
	const WORD GetCaPID(void) const;
	const CMediaData * GetPrivateData(void) const;

protected:
	virtual const bool StoreContents(const BYTE *pPayload);

	WORD m_wCaMethodID;			// Conditional Access Method ID
	WORD m_wCaPID;				// Conditional Access PID
	CMediaData m_PrivateData;	// Private Data
};


/////////////////////////////////////////////////////////////////////////////
// [0x48] Service �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CServiceDesc : public CBaseDesc
{
public:
	enum {DESC_TAG = 0x48U};

	CServiceDesc();
	CServiceDesc(const CServiceDesc &Operand);
	CServiceDesc & operator = (const CServiceDesc &Operand);
	
// CBaseDesc
	virtual void CopyDesc(const CBaseDesc *pOperand);
	virtual void Reset(void);

// CServiceDesc
	const BYTE GetServiceType(void) const;
	const DWORD GetProviderName(LPTSTR lpszDst, int MaxLength) const;
	const DWORD GetServiceName(LPTSTR lpszDst, int MaxLength) const;

protected:
	virtual const bool StoreContents(const BYTE *pPayload);

	BYTE m_byServiceType;			// Service Type
	TCHAR m_szProviderName[256];	// Service Provider Name
	TCHAR m_szServiceName[256];		// Service Name
};


/////////////////////////////////////////////////////////////////////////////
// [0x4D] Short Event �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CShortEventDesc : public CBaseDesc
{
public:
	enum {DESC_TAG = 0x4DU};

	CShortEventDesc();
	CShortEventDesc(const CShortEventDesc &Operand);
	CShortEventDesc & operator = (const CShortEventDesc &Operand);

// CBaseDesc
	virtual void CopyDesc(const CBaseDesc *pOperand);
	virtual void Reset(void);

// CShortEventDesc
	const DWORD GetLanguageCode(void) const;
	const DWORD GetEventName(LPTSTR lpszDst, int MaxLength) const;
	const DWORD GetEventDesc(LPTSTR lpszDst, int MaxLength) const;

protected:
	virtual const bool StoreContents(const BYTE *pPayload);

	DWORD m_dwLanguageCode;			// ISO639  Language Code
	TCHAR m_szEventName[256];		// Event Name
	TCHAR m_szEventDesc[256];		// Event Description
};


/////////////////////////////////////////////////////////////////////////////
// [0x52] Stream Identifier �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CStreamIdDesc : public CBaseDesc
{
public:
	enum {DESC_TAG = 0x52U};

	CStreamIdDesc();
	CStreamIdDesc(const CStreamIdDesc &Operand);
	CStreamIdDesc & operator = (const CStreamIdDesc &Operand);

// CBaseDesc
	virtual void CopyDesc(const CBaseDesc *pOperand);
	virtual void Reset(void);

// CStreamIdDesc
	const BYTE GetComponentTag(void) const;

protected:
	virtual const bool StoreContents(const BYTE *pPayload);

	BYTE m_byComponentTag;		// Component Tag
};


/////////////////////////////////////////////////////////////////////////////
// [0x40] Network Name �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CNetworkNameDesc : public CBaseDesc
{
public:
	enum {DESC_TAG = 0x40U};

	CNetworkNameDesc();
	CNetworkNameDesc(const CNetworkNameDesc &Operand);
	CNetworkNameDesc & operator = (const CNetworkNameDesc &Operand);

// CBaseDesc
	virtual void CopyDesc(const CBaseDesc *pOperand);
	virtual void Reset(void);

// CServiceDesc
	const DWORD GetNetworkName(LPTSTR pszName, int MaxLength) const;

protected:
	virtual const bool StoreContents(const BYTE *pPayload);

	TCHAR m_szNetworkName[32];
};


/////////////////////////////////////////////////////////////////////////////
// [0xFE] System Management �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CSystemManageDesc : public CBaseDesc
{
public:
	enum {DESC_TAG = 0xFEU};

	CSystemManageDesc();
	CSystemManageDesc(const CSystemManageDesc &Operand);
	CSystemManageDesc & operator = (const CSystemManageDesc &Operand);

// CBaseDesc
	virtual void CopyDesc(const CBaseDesc *pOperand);
	virtual void Reset(void);

// CServiceDesc
	const BYTE GetBroadcastingFlag(void) const;
	const BYTE GetBroadcastingID(void) const;
	const BYTE GetAdditionalBroadcastingID(void) const;

protected:
	virtual const bool StoreContents(const BYTE *pPayload);

	BYTE m_byBroadcastingFlag;
	BYTE m_byBroadcastingID;
	BYTE m_byAdditionalBroadcastingID;
};


/////////////////////////////////////////////////////////////////////////////
// [0xCD] TS Information �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CTSInfoDesc : public CBaseDesc
{
public:
	enum {DESC_TAG = 0xCDU};

	CTSInfoDesc();
	CTSInfoDesc(const CTSInfoDesc &Operand);
	CTSInfoDesc & operator = (const CTSInfoDesc &Operand);

// CBaseDesc
	virtual void CopyDesc(const CBaseDesc *pOperand);
	virtual void Reset(void);

// CTSInfoDesc
	const BYTE GetRemoteControlKeyID(void) const;
	const DWORD GetTSName(LPTSTR pszName, int MaxLength) const;

protected:
	virtual const bool StoreContents(const BYTE *pPayload);

	BYTE m_byRemoteControlKeyID;
	TCHAR m_szTSName[32];
};


/////////////////////////////////////////////////////////////////////////////
// [0x50] Component �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CComponentDesc : public CBaseDesc
{
public:
	enum {DESC_TAG = 0x50U};

	CComponentDesc();
	CComponentDesc(const CComponentDesc &Operand);
	CComponentDesc & operator = (const CComponentDesc &Operand);

// CBaseDesc
	virtual void CopyDesc(const CBaseDesc *pOperand);
	virtual void Reset(void);

// CComponentDesc
	const BYTE GetStreamContent(void) const;
	const BYTE GetComponentType(void) const;
	const BYTE GetComponentTag(void) const;
	const DWORD GetLanguageCode(void) const;
	const DWORD GetText(LPTSTR pszText, int MaxLength) const;

protected:
	virtual const bool StoreContents(const BYTE *pPayload);

	BYTE m_StreamContent;
	BYTE m_ComponentType;
	BYTE m_ComponentTag;
	DWORD m_LanguageCode;
	TCHAR m_szText[64];
};


/////////////////////////////////////////////////////////////////////////////
// [0xC4] Audio Component �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CAudioComponentDesc : public CBaseDesc
{
public:
	enum {DESC_TAG = 0xC4U};

	CAudioComponentDesc();
	CAudioComponentDesc(const CAudioComponentDesc &Operand);
	CAudioComponentDesc & operator = (const CAudioComponentDesc &Operand);

// CBaseDesc
	virtual void CopyDesc(const CBaseDesc *pOperand);
	virtual void Reset(void);

// CAudioComponentDesc
	const BYTE GetStreamContent(void) const;
	const BYTE GetComponentType(void) const;
	const BYTE GetComponentTag(void) const;
	const BYTE GetSimulcastGroupTag(void) const;
	const bool GetESMultiLingualFlag(void) const;
	const bool GetMainComponentFlag(void) const;
	const BYTE GetQualityIndicator(void) const;
	const BYTE GetSamplingRate(void) const;
	const DWORD GetLanguageCode(void) const;
	const DWORD GetLanguageCode2(void) const;
	const DWORD GetText(LPTSTR pszText, int MaxLength) const;

protected:
	virtual const bool StoreContents(const BYTE *pPayload);

	BYTE m_StreamContent;
	BYTE m_ComponentType;
	BYTE m_ComponentTag;
	BYTE m_StreamType;
	BYTE m_SimulcastGroupTag;
	bool m_bESMultiLingualFlag;
	bool m_bMainComponentFlag;
	BYTE m_QualityIndicator;
	BYTE m_SamplingRate;
	DWORD m_LanguageCode;
	DWORD m_LanguageCode2;
	TCHAR m_szText[64];
};


/////////////////////////////////////////////////////////////////////////////
// �L�q�q�u���b�N���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

class CDescBlock
{
public:
	CDescBlock();
	CDescBlock(const CDescBlock &Operand);
	~CDescBlock();
	CDescBlock & operator = (const CDescBlock &Operand);

	const WORD ParseBlock(const BYTE *pHexData, const WORD wDataLength);
	const CBaseDesc * ParseBlock(const BYTE *pHexData, const WORD wDataLength, const BYTE byTag);

	virtual void Reset(void);

	const WORD GetDescNum(void) const;
	const CBaseDesc * GetDescByIndex(const WORD wIndex = 0U) const;
	const CBaseDesc * GetDescByTag(const BYTE byTag) const;

protected:
	CBaseDesc * ParseDesc(const BYTE *pHexData, const WORD wDataLength);
	static CBaseDesc * CreateDescInstance(const BYTE byTag);

	vector<CBaseDesc *> m_DescArray;
};
