// MediaDecoder.cpp: CMediaDecoder �N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MediaDecoder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


//////////////////////////////////////////////////////////////////////
// CMediaDecoder �\�z/����
//////////////////////////////////////////////////////////////////////

CMediaDecoder::CMediaDecoder(IEventHandler *pEventHandler, const DWORD dwInputNum, const DWORD dwOutputNum)
	: m_pEventHandler(pEventHandler)
	, m_dwInputNum(dwInputNum)
	, m_dwOutputNum(dwOutputNum)
{
	// �o�̓t�B���^�z����N���A����
	::ZeroMemory(m_aOutputDecoder, sizeof(m_aOutputDecoder));
}

CMediaDecoder::~CMediaDecoder()
{
}

void CMediaDecoder::Reset()
{
}

void CMediaDecoder::ResetGraph(void)
{
	CBlockLock Lock(&m_DecoderLock);

	Reset();
	ResetDownstreamDecoder();
}

const DWORD CMediaDecoder::GetInputNum(void) const
{
	// ���͐���Ԃ�
	return m_dwInputNum;
}

const DWORD CMediaDecoder::GetOutputNum(void) const
{
	// �o�͐���Ԃ�
	return m_dwOutputNum;
}

const bool CMediaDecoder::SetOutputDecoder(CMediaDecoder *pDecoder, const DWORD dwOutputIndex, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	if (dwOutputIndex >= m_dwOutputNum)
		return false;

	// �o�̓t�B���^���Z�b�g����
	m_aOutputDecoder[dwOutputIndex].pDecoder = pDecoder;
	m_aOutputDecoder[dwOutputIndex].dwInputIndex = dwInputIndex;
	return true;
}

const bool CMediaDecoder::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	CBlockLock Lock(&m_DecoderLock);

	OutputMedia(pMediaData, dwInputIndex);
	return true;
}

const bool CMediaDecoder::OutputMedia(CMediaData *pMediaData, const DWORD dwOutptIndex)
{
	// �o�͏���
	if (dwOutptIndex >= m_dwOutputNum)
		return false;

	// ���̃t�B���^�Ƀf�[�^��n��
	if (m_aOutputDecoder[dwOutptIndex].pDecoder) {
		return m_aOutputDecoder[dwOutptIndex].pDecoder->InputMedia(pMediaData, m_aOutputDecoder[dwOutptIndex].dwInputIndex);
	}
	return false;
}

void CMediaDecoder::ResetDownstreamDecoder(void)
{
	// ���̃t�B���^�����Z�b�g����
	for (DWORD dwOutputIndex = 0UL ; dwOutputIndex < m_dwOutputNum ; dwOutputIndex++) {
		if (m_aOutputDecoder[dwOutputIndex].pDecoder)
			m_aOutputDecoder[dwOutputIndex].pDecoder->ResetGraph();
	}
}

const DWORD CMediaDecoder::SendDecoderEvent(const DWORD dwEventID, PVOID pParam)
{
	// �C�x���g��ʒm����
	if (m_pEventHandler==NULL)
		return 0;
	return m_pEventHandler->OnDecoderEvent(this, dwEventID, pParam);
}
