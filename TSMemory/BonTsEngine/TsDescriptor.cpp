// TsDescriptor.h: �L�q�q���b�p�[�N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TsEncode.h"
#include "TsDescriptor.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


/////////////////////////////////////////////////////////////////////////////
// �L�q�q�̊��N���X
/////////////////////////////////////////////////////////////////////////////

CBaseDesc::CBaseDesc()
{
	Reset();
}

CBaseDesc::CBaseDesc(const CBaseDesc &Operand)
{
	// �R�s�[�R���X�g���N�^
	CopyDesc(&Operand);
}

CBaseDesc::~CBaseDesc()
{

}

CBaseDesc & CBaseDesc::operator = (const CBaseDesc &Operand)
{
	// ������Z�q
	CopyDesc(&Operand);

	return *this;
}

void CBaseDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// �C���X�^���X�̃R�s�[
	m_byDescTag = pOperand->m_byDescTag;
	m_byDescLen = pOperand->m_byDescLen;
	m_bIsValid = pOperand->m_bIsValid;
}

const bool CBaseDesc::ParseDesc(const BYTE *pHexData, const WORD wDataLength)
{
	Reset();

	// ���ʃt�H�[�}�b�g���`�F�b�N
	if(!pHexData)return false;										// �f�[�^����
	else if(wDataLength < 2U)return false;							// �f�[�^���Œ�L�q�q�T�C�Y����
	else if(wDataLength < (WORD)(pHexData[1] + 2U))return false;	// �f�[�^���L�q�q�̃T�C�Y����������

	m_byDescTag = pHexData[0];
	m_byDescLen = pHexData[1];

	// �y�C���[�h���
	if (m_byDescLen > 0 && StoreContents(&pHexData[2])) {
		m_bIsValid = true;
	}

	return m_bIsValid;
}

const bool CBaseDesc::IsValid(void) const
{
	// �f�[�^���L��(��͍�)���ǂ�����Ԃ�
	return m_bIsValid;
}

const BYTE CBaseDesc::GetTag(void) const
{
	// �L�q�q�^�O��Ԃ�
	return m_byDescTag;
}

const BYTE CBaseDesc::GetLength(void) const
{
	// �L�q�q����Ԃ�
	return m_byDescLen;
}

void CBaseDesc::Reset(void)
{
	// ��Ԃ��N���A����
	m_byDescTag = 0x00U;
	m_byDescLen = 0U;
	m_bIsValid = false;
}

const bool CBaseDesc::StoreContents(const BYTE *pPayload)
{
	// �f�t�H���g�̎����ł͉������Ȃ�
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x09] Conditional Access �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CCaMethodDesc::CCaMethodDesc()
	: CBaseDesc()
{
	Reset();
}

CCaMethodDesc::CCaMethodDesc(const CCaMethodDesc &Operand)
{
	CopyDesc(&Operand);
}

CCaMethodDesc & CCaMethodDesc::operator = (const CCaMethodDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CCaMethodDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// �C���X�^���X�̃R�s�[
	CBaseDesc::CopyDesc(pOperand);

	const CCaMethodDesc *pSrcDesc = dynamic_cast<const CCaMethodDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_wCaMethodID = pSrcDesc->m_wCaMethodID;
		m_wCaPID = pSrcDesc->m_wCaPID;
		m_PrivateData = pSrcDesc->m_PrivateData;
	}
}

void CCaMethodDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_wCaMethodID = 0x0000U;		// Conditional Access Method ID
	m_wCaPID = 0xFFFFU;				// Conditional Access PID
	m_PrivateData.ClearSize();		// Private Data
}

const WORD CCaMethodDesc::GetCaMethodID(void) const
{
	// Conditional Access Method ID ��Ԃ�
	return m_wCaMethodID;
}

const WORD CCaMethodDesc::GetCaPID(void) const
{
	// Conditional Access PID
	return m_wCaPID;
}

const CMediaData * CCaMethodDesc::GetPrivateData(void) const
{
	// Private Data ��Ԃ�
	return &m_PrivateData;
}

const bool CCaMethodDesc::StoreContents(const BYTE *pPayload)
{
	// �t�H�[�}�b�g���`�F�b�N
	if(m_byDescTag != DESC_TAG)return false;							// �^�O���s��
	else if(m_byDescLen < 4U)return false;								// CA���\�b�h�L�q�q�̍ŏ��T�C�Y��4
	else if((pPayload[2] & 0xE0U) != 0xE0U)return false;				// �Œ�r�b�g���s��

	// �L�q�q�����
	m_wCaMethodID = (WORD)pPayload[0] << 8 | (WORD)pPayload[1];			// +0,1	Conditional Access Method ID
	m_wCaPID = (WORD)(pPayload[2] & 0x1FU) << 8 | (WORD)pPayload[3];	// +2,3	Conditional Access PID
	m_PrivateData.SetData(&pPayload[4], m_byDescLen - 4U);				// +4-	Private Data

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x48] Service �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CServiceDesc::CServiceDesc()
	: CBaseDesc()
{
	Reset();
}

CServiceDesc::CServiceDesc(const CServiceDesc &Operand)
{
	CopyDesc(&Operand);
}

CServiceDesc & CServiceDesc::operator = (const CServiceDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CServiceDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// �C���X�^���X�̃R�s�[
	CBaseDesc::CopyDesc(pOperand);

	const CServiceDesc *pSrcDesc = dynamic_cast<const CServiceDesc *>(pOperand);
	
	if (pSrcDesc && pSrcDesc != this) {
		m_byServiceType = pSrcDesc->m_byServiceType;
		::lstrcpy(m_szProviderName, pSrcDesc->m_szProviderName);
		::lstrcpy(m_szServiceName, pSrcDesc->m_szServiceName);
	}
}

void CServiceDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_byServiceType = 0x00U;			// Service Type
	m_szProviderName[0] = TEXT('\0');	// Service Provider Name
	m_szServiceName[0] = TEXT('\0');	// Service Name
}

const BYTE CServiceDesc::GetServiceType(void) const
{
	// Service Type��Ԃ�
	return m_byServiceType;
}

const DWORD CServiceDesc::GetProviderName(LPTSTR lpszDst, int MaxLength) const
{
	// Service Provider Name��Ԃ�
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szProviderName, MaxLength);

	return ::lstrlen(m_szProviderName);
}

const DWORD CServiceDesc::GetServiceName(LPTSTR lpszDst, int MaxLength) const
{
	// Service Provider Name��Ԃ�
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szServiceName, MaxLength);

	return ::lstrlen(m_szServiceName);
}

const bool CServiceDesc::StoreContents(const BYTE *pPayload)
{
	// �t�H�[�}�b�g���`�F�b�N
	if(m_byDescTag != DESC_TAG)return false;	// �^�O���s��
	else if(m_byDescLen < 3U)return false;		// �T�[�r�X�L�q�q�̃T�C�Y�͍Œ�3

	// �L�q�q�����
	m_byServiceType = pPayload[0];				// +0	Service Type

	int Pos = 1, Length;

	// Provider Name
	Length = pPayload[Pos++];
	if (Length > 0) {
		if (Pos + Length >= m_byDescLen)
			return false;
		CAribString::AribToString(m_szProviderName, sizeof(m_szProviderName) / sizeof(TCHAR), &pPayload[Pos], Length);
		Pos += Length;
	}

	// Service Name
	Length = pPayload[Pos++];
	if (Length > 0) {
		if (Pos + Length > m_byDescLen)
			return false;
		CAribString::AribToString(m_szServiceName, sizeof(m_szServiceName) / sizeof(TCHAR), &pPayload[Pos], Length);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x4D] Short Event �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CShortEventDesc::CShortEventDesc()
	: CBaseDesc()
{
	Reset();
}

CShortEventDesc::CShortEventDesc(const CShortEventDesc &Operand)
{
	CopyDesc(&Operand);
}

CShortEventDesc & CShortEventDesc::operator = (const CShortEventDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CShortEventDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// �C���X�^���X�̃R�s�[
	CBaseDesc::CopyDesc(pOperand);

	const CShortEventDesc *pSrcDesc = dynamic_cast<const CShortEventDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_dwLanguageCode = pSrcDesc->m_dwLanguageCode;
		::lstrcpy(m_szEventName, pSrcDesc->m_szEventName);
		::lstrcpy(m_szEventDesc, pSrcDesc->m_szEventDesc);
	}
}

void CShortEventDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_dwLanguageCode = 0UL;			// ISO639  Language Code
	m_szEventName[0] = TEXT('\0');	// Event Name
	m_szEventDesc[0] = TEXT('\0');	// Event Description
}

const DWORD CShortEventDesc::GetLanguageCode(void) const
{
	// Language Code��Ԃ�
	return m_dwLanguageCode;
}

const DWORD CShortEventDesc::GetEventName(LPTSTR lpszDst, int MaxLength) const
{
	// Event Name��Ԃ�
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szEventName, MaxLength);

	return ::lstrlen(m_szEventName);
}

const DWORD CShortEventDesc::GetEventDesc(LPTSTR lpszDst, int MaxLength) const
{
	// Event Description��Ԃ�
	if (lpszDst && MaxLength > 0)
		::lstrcpyn(lpszDst, m_szEventDesc, MaxLength);

	return ::lstrlen(m_szEventDesc);
}

const bool CShortEventDesc::StoreContents(const BYTE *pPayload)
{
	// �t�H�[�}�b�g���`�F�b�N
	if(m_byDescTag != DESC_TAG)return false;	// �^�O���s��
	else if(m_byDescLen < 5U)return false;		// Short Event�L�q�q�̃T�C�Y�͍Œ�5

	// �L�q�q�����
	m_dwLanguageCode = ((DWORD)pPayload[0] << 16) | ((DWORD)pPayload[1] << 8) | (DWORD)pPayload[2];		// +0 - +2	ISO639  Language Code

	int Pos = 3, Length;

	// Event Name
	Length = pPayload[Pos++];
	if (Length > 0) {
		if (Pos + Length >= m_byDescLen)
			return false;
		CAribString::AribToString(m_szEventName, sizeof(m_szEventName) / sizeof(TCHAR), &pPayload[Pos], Length);
		Pos += Length;
	}

	// Event Description
	Length = pPayload[Pos++];
	if (Length > 0) {
		if (Pos + Length > m_byDescLen)
			return false;
		CAribString::AribToString(m_szEventDesc, sizeof(m_szEventDesc) / sizeof(TCHAR), &pPayload[Pos], Length);
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x52] Stream Identifier �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CStreamIdDesc::CStreamIdDesc()
	: CBaseDesc()
{
	Reset();
}

CStreamIdDesc::CStreamIdDesc(const CStreamIdDesc &Operand)
{
	CopyDesc(&Operand);
}

CStreamIdDesc & CStreamIdDesc::operator = (const CStreamIdDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CStreamIdDesc::CopyDesc(const CBaseDesc *pOperand)
{
	// �C���X�^���X�̃R�s�[
	CBaseDesc::CopyDesc(pOperand);

	const CStreamIdDesc *pSrcDesc = dynamic_cast<const CStreamIdDesc *>(pOperand);

	if (pSrcDesc) {
		m_byComponentTag = pSrcDesc->m_byComponentTag;
	}
}

void CStreamIdDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_byComponentTag = 0x00U;	// Component Tag
}

const BYTE CStreamIdDesc::GetComponentTag(void) const
{
	// Component Tag ��Ԃ�
	return m_byComponentTag;
}

const bool CStreamIdDesc::StoreContents(const BYTE *pPayload)
{
	// �t�H�[�}�b�g���`�F�b�N
	if(m_byDescTag != DESC_TAG)return false;	// �^�O���s��
	else if(m_byDescLen != 1U)return false;		// �X�g���[��ID�L�q�q�̃T�C�Y�͏��1

	// �L�q�q�����
	m_byComponentTag = pPayload[0];				// +0	Component Tag

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x40] Network Name �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CNetworkNameDesc::CNetworkNameDesc()
	: CBaseDesc()
{
	Reset();
}

CNetworkNameDesc::CNetworkNameDesc(const CNetworkNameDesc &Operand)
{
	CopyDesc(&Operand);
}

CNetworkNameDesc & CNetworkNameDesc::operator = (const CNetworkNameDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CNetworkNameDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CNetworkNameDesc *pSrcDesc = dynamic_cast<const CNetworkNameDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		::lstrcpy(m_szNetworkName, pSrcDesc->m_szNetworkName);
	}
}

void CNetworkNameDesc::Reset(void)
{
	CBaseDesc::Reset();
	m_szNetworkName[0] = '\0';
}

const DWORD CNetworkNameDesc::GetNetworkName(LPTSTR pszName, int MaxLength) const
{
	if (pszName && MaxLength > 0)
		::lstrcpyn(pszName, m_szNetworkName, MaxLength);
	return ::lstrlen(m_szNetworkName);
}

const bool CNetworkNameDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG)
		return false;

	CAribString::AribToString(m_szNetworkName, sizeof(m_szNetworkName) / sizeof(TCHAR), &pPayload[0], m_byDescLen);

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xFE] System Management �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CSystemManageDesc::CSystemManageDesc()
	: CBaseDesc()
{
	Reset();
}

CSystemManageDesc::CSystemManageDesc(const CSystemManageDesc &Operand)
{
	CopyDesc(&Operand);
}

CSystemManageDesc & CSystemManageDesc::operator = (const CSystemManageDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CSystemManageDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CSystemManageDesc *pSrcDesc = dynamic_cast<const CSystemManageDesc *>(pOperand);

	if (pSrcDesc) {
		m_byBroadcastingFlag = pSrcDesc->m_byBroadcastingFlag;
		m_byBroadcastingID = pSrcDesc->m_byBroadcastingID;
		m_byAdditionalBroadcastingID = pSrcDesc->m_byAdditionalBroadcastingID;
	}
}

void CSystemManageDesc::Reset(void)
{
	CBaseDesc::Reset();
	m_byBroadcastingFlag = 0;
	m_byBroadcastingID = 0;
	m_byAdditionalBroadcastingID = 0;
}

const BYTE CSystemManageDesc::GetBroadcastingFlag(void) const
{
	return m_byBroadcastingFlag;
}

const BYTE CSystemManageDesc::GetBroadcastingID(void) const
{
	return m_byBroadcastingID;
}

const BYTE CSystemManageDesc::GetAdditionalBroadcastingID(void) const
{
	return m_byAdditionalBroadcastingID;
}

const bool CSystemManageDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 2)
		return false;

	m_byBroadcastingFlag = (pPayload[0] & 0xC0) >> 6;
	m_byBroadcastingID = (pPayload[0] & 0x3F);
	m_byAdditionalBroadcastingID = pPayload[1];

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xCD] TS Information �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CTSInfoDesc::CTSInfoDesc()
	: CBaseDesc()
{
	Reset();
}

CTSInfoDesc::CTSInfoDesc(const CTSInfoDesc &Operand)
{
	CopyDesc(&Operand);
}

CTSInfoDesc & CTSInfoDesc::operator = (const CTSInfoDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CTSInfoDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CTSInfoDesc *pSrcDesc = dynamic_cast<const CTSInfoDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_byRemoteControlKeyID = pSrcDesc->m_byRemoteControlKeyID;
		::lstrcpy(m_szTSName, pSrcDesc->m_szTSName);
	}
}

void CTSInfoDesc::Reset(void)
{
	CBaseDesc::Reset();
	m_byRemoteControlKeyID = 0;
	m_szTSName[0] = '\0';
}

const BYTE CTSInfoDesc::GetRemoteControlKeyID(void) const
{
	return m_byRemoteControlKeyID;
}

const DWORD CTSInfoDesc::GetTSName(LPTSTR pszName, int MaxLength) const
{
	if (pszName && MaxLength > 0)
		::lstrcpyn(pszName, m_szTSName, MaxLength);
	return ::lstrlen(m_szTSName);
}

const bool CTSInfoDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 2)
		return false;

	m_byRemoteControlKeyID = pPayload[0];

	BYTE Length = pPayload[1] >> 2;
	if (2 + Length > m_byDescLen)
		return false;

	CAribString::AribToString(m_szTSName, sizeof(m_szTSName) / sizeof(TCHAR), &pPayload[2], Length);

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0x50] Component �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CComponentDesc::CComponentDesc()
	: CBaseDesc()
{
	Reset();
}

CComponentDesc::CComponentDesc(const CComponentDesc &Operand)
{
	CopyDesc(&Operand);
}

CComponentDesc & CComponentDesc::operator = (const CComponentDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CComponentDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CComponentDesc *pSrcDesc = dynamic_cast<const CComponentDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_StreamContent = pSrcDesc->m_StreamContent;
		m_ComponentType = pSrcDesc->m_ComponentType;
		m_ComponentTag = pSrcDesc->m_ComponentTag;
		m_LanguageCode = pSrcDesc->m_LanguageCode;
		::lstrcpy(m_szText, pSrcDesc->m_szText);
	}
}

void CComponentDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_StreamContent = 0;
	m_ComponentType = 0;
	m_ComponentTag = 0;
	m_LanguageCode = 0;
	m_szText[0] = '\0';
}

const BYTE CComponentDesc::GetStreamContent(void) const
{
	return m_StreamContent;
}

const BYTE CComponentDesc::GetComponentType(void) const
{
	return m_ComponentType;
}

const BYTE CComponentDesc::GetComponentTag(void) const
{
	return m_ComponentTag;
}

const DWORD CComponentDesc::GetLanguageCode(void) const
{
	return m_LanguageCode;
}

const DWORD CComponentDesc::GetText(LPTSTR pszText, int MaxLength) const
{
	if (pszText && MaxLength > 0)
		::lstrcpyn(pszText, m_szText, MaxLength);
	return ::lstrlen(m_szText);
}

const bool CComponentDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 6)
		return false;

	m_StreamContent = pPayload[0] & 0x0F;
	if (m_StreamContent != 0x01)
		return false;
	m_ComponentType = pPayload[1];
	m_ComponentTag = pPayload[2];
	m_LanguageCode = (pPayload[3] << 16) | (pPayload[4] << 8) | pPayload[5];
	if (m_byDescLen > 6)
		CAribString::AribToString(m_szText, sizeof(m_szText) / sizeof(TCHAR), &pPayload[6], min(m_byDescLen - 6, 16));
	else
		m_szText[0]='\0';
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// [0xC4] Audio Component �L�q�q���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CAudioComponentDesc::CAudioComponentDesc()
	: CBaseDesc()
{
	Reset();
}

CAudioComponentDesc::CAudioComponentDesc(const CAudioComponentDesc &Operand)
{
	CopyDesc(&Operand);
}

CAudioComponentDesc & CAudioComponentDesc::operator = (const CAudioComponentDesc &Operand)
{
	CopyDesc(&Operand);

	return *this;
}

void CAudioComponentDesc::CopyDesc(const CBaseDesc *pOperand)
{
	CBaseDesc::CopyDesc(pOperand);

	const CAudioComponentDesc *pSrcDesc = dynamic_cast<const CAudioComponentDesc *>(pOperand);

	if (pSrcDesc && pSrcDesc != this) {
		m_StreamContent = pSrcDesc->m_StreamContent;
		m_ComponentType = pSrcDesc->m_ComponentType;
		m_ComponentTag = pSrcDesc->m_ComponentTag;
		m_StreamType = pSrcDesc->m_StreamType;
		m_SimulcastGroupTag = pSrcDesc->m_SimulcastGroupTag;
		m_bESMultiLingualFlag = pSrcDesc->m_bESMultiLingualFlag;
		m_bMainComponentFlag = pSrcDesc->m_bMainComponentFlag;
		m_QualityIndicator = pSrcDesc->m_QualityIndicator;
		m_SamplingRate = pSrcDesc->m_SamplingRate;
		m_LanguageCode = pSrcDesc->m_LanguageCode;
		m_LanguageCode2 = pSrcDesc->m_LanguageCode2;
		::lstrcpy(m_szText, pSrcDesc->m_szText);
	}
}

void CAudioComponentDesc::Reset(void)
{
	CBaseDesc::Reset();

	m_StreamContent = 0;
	m_ComponentType = 0;
	m_ComponentTag = 0;
	m_StreamType = 0;
	m_SimulcastGroupTag = 0;
	m_bESMultiLingualFlag = false;
	m_bMainComponentFlag = false;
	m_QualityIndicator = 0;
	m_SamplingRate = 0;
	m_LanguageCode = 0;
	m_LanguageCode2 = 0;
	m_szText[0] = '\0';
}

const BYTE CAudioComponentDesc::GetStreamContent(void) const
{
	return m_StreamContent;
}

const BYTE CAudioComponentDesc::GetComponentType(void) const
{
	return m_ComponentType;
}

const BYTE CAudioComponentDesc::GetComponentTag(void) const
{
	return m_ComponentTag;
}

const BYTE CAudioComponentDesc::GetSimulcastGroupTag(void) const
{
	return m_SimulcastGroupTag;
}

const bool CAudioComponentDesc::GetESMultiLingualFlag(void) const
{
	return m_bESMultiLingualFlag;
}

const bool CAudioComponentDesc::GetMainComponentFlag(void) const
{
	return m_bMainComponentFlag;
}

const BYTE CAudioComponentDesc::GetQualityIndicator(void) const
{
	return m_QualityIndicator;
}

const BYTE CAudioComponentDesc::GetSamplingRate(void) const
{
	return m_SamplingRate;
}

const DWORD CAudioComponentDesc::GetLanguageCode(void) const
{
	return m_LanguageCode;
}

const DWORD CAudioComponentDesc::GetLanguageCode2(void) const
{
	return m_LanguageCode2;
}

const DWORD CAudioComponentDesc::GetText(LPTSTR pszText, int MaxLength) const
{
	if (pszText && MaxLength > 0)
		::lstrcpyn(pszText, m_szText, MaxLength);
	return ::lstrlen(m_szText);
}

const bool CAudioComponentDesc::StoreContents(const BYTE *pPayload)
{
	if (m_byDescTag != DESC_TAG || m_byDescLen < 9)
		return false;

	m_StreamContent = pPayload[0] & 0x0F;
	if (m_StreamContent != 0x02)
		return false;
	m_ComponentType = pPayload[1];
	m_ComponentTag = pPayload[2];
	m_StreamType = pPayload[3];
	m_SimulcastGroupTag = pPayload[4];
	m_bESMultiLingualFlag = (pPayload[5] & 0x80) != 0;
	m_bMainComponentFlag = (pPayload[5] & 0x40) != 0;
	m_QualityIndicator = (pPayload[5] & 0x30) >> 4;
	m_SamplingRate = (pPayload[5] & 0x0E) >> 1;
	m_LanguageCode = (pPayload[6] << 16) | (pPayload[7] << 8) | pPayload[8];
	int Pos = 9;
	if (m_bESMultiLingualFlag) {
		if (Pos + 3 > m_byDescLen)
			return false;
		m_LanguageCode2 = (pPayload[Pos] << 16) | (pPayload[Pos + 1] << 8) | pPayload[Pos + 2];
		Pos += 3;
	}
	if (Pos < m_byDescLen)
		CAribString::AribToString(m_szText, sizeof(m_szText) / sizeof(TCHAR), &pPayload[Pos], min(m_byDescLen - Pos, 33));
	else
		m_szText[0]='\0';
	return true;
}


/////////////////////////////////////////////////////////////////////////////
// �L�q�q�u���b�N���ۉ��N���X
/////////////////////////////////////////////////////////////////////////////

CDescBlock::CDescBlock()
{

}

CDescBlock::CDescBlock(const CDescBlock &Operand)
{
	*this = Operand;
}

CDescBlock::~CDescBlock()
{
	Reset();
}

CDescBlock & CDescBlock::operator = (const CDescBlock &Operand)
{
	if (&Operand == this)
		return *this;

	// �C���X�^���X�̃R�s�[
	Reset();
	m_DescArray.resize(Operand.m_DescArray.size());

	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++) {
		m_DescArray[Index] = CreateDescInstance(Operand.m_DescArray[Index]->GetTag());
		m_DescArray[Index]->CopyDesc(Operand.m_DescArray[Index]);
	}

	return *this;
}

const WORD CDescBlock::ParseBlock(const BYTE *pHexData, const WORD wDataLength)
{
	if (!pHexData || wDataLength < 2U)
		return 0U;

	// ��Ԃ��N���A
	Reset();

	// �w�肳�ꂽ�u���b�N�Ɋ܂܂��L�q�q����͂���
	WORD wPos = 0UL;

	do {
		CBaseDesc *pNewDesc;

		// �u���b�N����͂���
		if (!(pNewDesc = ParseDesc(&pHexData[wPos], wDataLength - wPos)))
			break;

		// ���X�g�ɒǉ�����
		m_DescArray.push_back(pNewDesc);

		// �ʒu�X�V
		wPos += (pNewDesc->GetLength() + 2U);
	} while (wPos < wDataLength);

	return m_DescArray.size();
}

const CBaseDesc * CDescBlock::ParseBlock(const BYTE *pHexData, const WORD wDataLength, const BYTE byTag)
{
	// �w�肳�ꂽ�u���b�N�Ɋ܂܂��L�q�q����͂��Ďw�肳�ꂽ�^�O�̋L�q�q��Ԃ�
	return (ParseBlock(pHexData, wDataLength))? GetDescByTag(byTag) : NULL;
}

void CDescBlock::Reset(void)
{
	// �S�ẴC���X�^���X���J������
	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++) {
		delete m_DescArray[Index];
	}

	m_DescArray.clear();
}

const WORD CDescBlock::GetDescNum(void) const
{
	// �L�q�q�̐���Ԃ�
	return m_DescArray.size();
}

const CBaseDesc * CDescBlock::GetDescByIndex(const WORD wIndex) const
{
	// �C���f�b�N�X�Ŏw�肵���L�q�q��Ԃ�
	return (wIndex < m_DescArray.size())? m_DescArray[wIndex] : NULL;
}

const CBaseDesc * CDescBlock::GetDescByTag(const BYTE byTag) const
{
	// �w�肵���^�O�Ɉ�v����L�q�q��Ԃ�
	for (size_t Index = 0 ; Index < m_DescArray.size() ; Index++){
		if (m_DescArray[Index]->GetTag() == byTag)
			return m_DescArray[Index];
	}

	return NULL;
}

CBaseDesc * CDescBlock::ParseDesc(const BYTE *pHexData, const WORD wDataLength)
{
	if (!pHexData || wDataLength < 2U)
		return NULL;

	// �^�O�ɑΉ������C���X�^���X�𐶐�����
	CBaseDesc *pNewDesc = CreateDescInstance(pHexData[0]);

	/*
	// �������s��
	if(!pNewDesc)return NULL;
	*/

	// �L�q�q����͂���
	if (!pNewDesc->ParseDesc(pHexData, wDataLength)) {
		// �G���[����
		delete pNewDesc;
		return NULL;
	}

	return pNewDesc;
}

CBaseDesc * CDescBlock::CreateDescInstance(const BYTE byTag)
{
	// �^�O�ɑΉ������C���X�^���X�𐶐�����
	switch (byTag) {
	case CCaMethodDesc::DESC_TAG		: return new CCaMethodDesc;
	case CServiceDesc::DESC_TAG			: return new CServiceDesc;
	case CShortEventDesc::DESC_TAG		: return new CShortEventDesc;
	case CStreamIdDesc::DESC_TAG		: return new CStreamIdDesc;
	case CNetworkNameDesc::DESC_TAG		: return new CNetworkNameDesc;
	case CSystemManageDesc::DESC_TAG	: return new CSystemManageDesc;
	case CTSInfoDesc::DESC_TAG			: return new CTSInfoDesc;
	case CComponentDesc::DESC_TAG		: return new CComponentDesc;
	case CAudioComponentDesc::DESC_TAG	: return new CAudioComponentDesc;
	default								: return new CBaseDesc;
	}
}
