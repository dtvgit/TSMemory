// TsEncode.cpp: TS�G���R�[�h�N���X�̃C���v�������e�[�V����
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <mbstring.h>
#include "TsEncode.h"


//////////////////////////////////////////////////////////////////////
// CAribString �N���X�̍\�z/����
//////////////////////////////////////////////////////////////////////

static const bool abCharSizeTable[] =
{
	false,	// CODE_UNKNOWN					�s���ȃO���t�B�b�N�Z�b�g(��Ή�)
	true,	// CODE_KANJI					Kanji
	false,	// CODE_ALPHANUMERIC			Alphanumeric
	false,	// CODE_HIRAGANA				Hiragana
	false,	// CODE_KATAKANA				Katakana
	false,	// CODE_MOSAIC_A				Mosaic A
	false,	// CODE_MOSAIC_B				Mosaic B
	false,	// CODE_MOSAIC_C				Mosaic C
	false,	// CODE_MOSAIC_D				Mosaic D
	false,	// CODE_PROP_ALPHANUMERIC		Proportional Alphanumeric
	false,	// CODE_PROP_HIRAGANA			Proportional Hiragana
	false,	// CODE_PROP_KATAKANA			Proportional Katakana
	false,	// CODE_JIS_X0201_KATAKANA		JIS X 0201 Katakana
	true,	// CODE_JIS_KANJI_PLANE_1		JIS compatible Kanji Plane 1
	true,	// CODE_JIS_KANJI_PLANE_2		JIS compatible Kanji Plane 2
	true	// CODE_ADDITIONAL_SYMBOLS		Additional symbols
};

const DWORD CAribString::AribToString(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen)
{
	// ARIB STD-B24 Part1 �� Shift-JIS / Unicode�ϊ�
	CAribString WorkObject;

	return WorkObject.AribToStringInternal(lpszDst, dwDstLen, pSrcData, dwSrcLen);
}

const DWORD CAribString::AribToStringInternal(TCHAR *lpszDst, const DWORD dwDstLen, const BYTE *pSrcData, const DWORD dwSrcLen)
{
	if (pSrcData == NULL || lpszDst == NULL || dwDstLen == 0)
		return 0UL;

	DWORD dwSrcPos = 0UL, dwDstPos = 0UL;
	DWORD Length;

	// ��ԏ����ݒ�
	m_byEscSeqCount = 0U;
	m_pSingleGL = NULL;

	m_CodeG[0] = CODE_KANJI;
	m_CodeG[1] = CODE_ALPHANUMERIC;
	m_CodeG[2] = CODE_HIRAGANA;
	m_CodeG[3] = CODE_KATAKANA;

	m_pLockingGL = &m_CodeG[0];
	m_pLockingGR = &m_CodeG[2];

	while (dwSrcPos < dwSrcLen && dwDstPos < dwDstLen - 1) {
		if (!m_byEscSeqCount) {
			// GL/GR�̈�
			if ((pSrcData[dwSrcPos] >= 0x21U) && (pSrcData[dwSrcPos] <= 0x7EU)) {
				// GL�̈�
				const CODE_SET CurCodeSet = (m_pSingleGL)? *m_pSingleGL : *m_pLockingGL;
				m_pSingleGL = NULL;

				if (abCharSizeTable[CurCodeSet]) {
					// 2�o�C�g�R�[�h
					if ((dwSrcLen - dwSrcPos) < 2UL)
						break;

					Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos - 1, ((WORD)pSrcData[dwSrcPos + 0] << 8) | (WORD)pSrcData[dwSrcPos + 1], CurCodeSet);
					if (Length == 0)
						break;
					dwDstPos += Length;
					dwSrcPos++;
				} else {
					// 1�o�C�g�R�[�h
					Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos - 1, (WORD)pSrcData[dwSrcPos], CurCodeSet);
					if (Length == 0)
						break;
					dwDstPos += Length;
				}
			} else if ((pSrcData[dwSrcPos] >= 0xA1U) && (pSrcData[dwSrcPos] <= 0xFEU)) {
				// GR�̈�
				const CODE_SET CurCodeSet = *m_pLockingGR;

				if (abCharSizeTable[CurCodeSet]) {
					// 2�o�C�g�R�[�h
					if ((dwSrcLen - dwSrcPos) < 2UL) break;

					Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos -1, ((WORD)(pSrcData[dwSrcPos + 0] & 0x7FU) << 8) | (WORD)(pSrcData[dwSrcPos + 1] & 0x7FU), CurCodeSet);
					if (Length == 0)
						break;
					dwDstPos += Length;
					dwSrcPos++;
				} else {
					// 1�o�C�g�R�[�h
					Length = ProcessCharCode(&lpszDst[dwDstPos], dwDstLen - dwDstPos - 1, (WORD)(pSrcData[dwSrcPos] & 0x7FU), CurCodeSet);
					if (Length == 0)
						break;
					dwDstPos += Length;
				}
			} else {
				// ����R�[�h
				switch (pSrcData[dwSrcPos]) {
				case 0x0FU	: LockingShiftGL(0U);				break;	// LS0
				case 0x0EU	: LockingShiftGL(1U);				break;	// LS1
				case 0x19U	: SingleShiftGL(2U);				break;	// SS2
				case 0x1DU	: SingleShiftGL(3U);				break;	// SS3
				case 0x1BU	: m_byEscSeqCount = 1U;				break;	// ESC
				case 0x20U	:
				case 0xA0U	: lpszDst[dwDstPos++] = TEXT(' ');	break;	// SP
				default		: break;	// ��Ή�
				}
			}
		} else {
			// �G�X�P�[�v�V�[�P���X����
			ProcessEscapeSeq(pSrcData[dwSrcPos]);
		}

		dwSrcPos++;
	}

	// �I�[����
	lpszDst[dwDstPos] = TEXT('\0');

	return dwDstPos;
}

inline const int CAribString::ProcessCharCode(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode, const CODE_SET CodeSet)
{
	switch (CodeSet) {
	case CODE_KANJI	:
	case CODE_JIS_KANJI_PLANE_1 :
	case CODE_JIS_KANJI_PLANE_2 :
		// �����R�[�h�o��
		return PutKanjiChar(lpszDst, dwDstLen, wCode);

	case CODE_ALPHANUMERIC :
	case CODE_PROP_ALPHANUMERIC :
		// �p�����R�[�h�o��
		return PutAlphanumericChar(lpszDst, dwDstLen, wCode);

	case CODE_HIRAGANA :
	case CODE_PROP_HIRAGANA :
		// �Ђ炪�ȃR�[�h�o��
		return PutHiraganaChar(lpszDst, dwDstLen, wCode);

	case CODE_PROP_KATAKANA :
	case CODE_KATAKANA :
		// �J�^�J�i�R�[�h�o��
		return PutKatakanaChar(lpszDst, dwDstLen, wCode);

	case CODE_JIS_X0201_KATAKANA :
		// JIS�J�^�J�i�R�[�h�o��
		return PutJisKatakanaChar(lpszDst, dwDstLen, wCode);

	case CODE_ADDITIONAL_SYMBOLS :
		// �ǉ��V���{���R�[�h�o��
		return PutSymbolsChar(lpszDst, dwDstLen, wCode);
	}

	return 0;
}

inline const int CAribString::PutKanjiChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// JIS��Shift-JIS�����R�[�h�ϊ�
	const WORD wShiftJIS = ::_mbcjistojms(wCode);

#ifdef _UNICODE
	// Shift-JIS �� UNICODE
	char cShiftJIS[2];
	cShiftJIS[0] = (char)(wShiftJIS >> 8);
	cShiftJIS[1] = (char)(wShiftJIS & 0x00FFU);
	return ::MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, cShiftJIS, 2, lpszDst, dwDstLen);
#else
	// Shift-JIS �� Shift-JIS
	if (dwDstLen < 2)
		return 0;
	lpszDst[0] = (char)(wShiftJIS >> 8);
	lpszDst[1] = (char)(wShiftJIS & 0x00FFU);

	return 2;
#endif
}

inline const int CAribString::PutAlphanumericChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// �p���������R�[�h�ϊ�
	static const TCHAR *acAlphanumericTable = 
		TEXT("�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@")
		TEXT("�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@")
		TEXT("�@�I�h���������f�i�j���{�C�|�D�^")
		TEXT("�O�P�Q�R�S�T�U�V�W�X�F�G�������H")
		TEXT("���`�a�b�c�d�e�f�g�h�i�j�k�l�m�n")
		TEXT("�o�p�q�r�s�t�u�v�w�x�y�m���n�O�Q")
		TEXT("�@������������������������������")
		TEXT("�����������������������o�b�p�P�@");

#ifdef _UNICODE
	if (dwDstLen < 1)
		return 0;
	lpszDst[0] = acAlphanumericTable[wCode];

	return 1;
#else
	if (dwDstLen < 2)
		return 0;
	lpszDst[0] = acAlphanumericTable[wCode * 2U + 0U];
	lpszDst[1] = acAlphanumericTable[wCode * 2U + 1U];

	return 2;
#endif
}

inline const int CAribString::PutHiraganaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// �Ђ炪�ȕ����R�[�h�ϊ�
	static const TCHAR *acHiraganaTable = 
		TEXT("�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@")
		TEXT("�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@")
		TEXT("�@������������������������������")
		TEXT("��������������������������������")
		TEXT("���������ÂĂłƂǂȂɂʂ˂̂�")
		TEXT("�΂ςЂт҂ӂԂՂւׂ؂قڂۂ܂�")
		TEXT("�ނ߂���������������")
		TEXT("������@�@�@�T�U�[�B�u�v�A�E�@");
	
#ifdef _UNICODE
	if (dwDstLen < 1)
		return 0;
	lpszDst[0] = acHiraganaTable[wCode];

	return 1;
#else
	if (dwDstLen < 2)
		return 0;
	lpszDst[0] = acHiraganaTable[wCode * 2U + 0U];
	lpszDst[1] = acHiraganaTable[wCode * 2U + 1U];

	return 2;
#endif
}

inline const int CAribString::PutKatakanaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// �J�^�J�i�p���������R�[�h�ϊ�
	static const TCHAR *acKatakanaTable = 
		TEXT("�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@")
		TEXT("�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@")
		TEXT("�@�@�A�B�C�D�E�F�G�H�I�J�K�L�M�N")
		TEXT("�O�P�Q�R�S�T�U�V�W�X�Y�Z�[�\�]�^")
		TEXT("�_�`�a�b�c�d�e�f�g�h�i�j�k�l�m�n")
		TEXT("�o�p�q�r�s�t�u�v�w�x�y�z�{�|�}�~")
		TEXT("��������������������������������")
		TEXT("���������������R�S�[�B�u�v�A�E�@");

#ifdef _UNICODE
	if (dwDstLen < 1)
		return 0;
	lpszDst[0] = acKatakanaTable[wCode];

	return 1;
#else
	if (dwDstLen < 2)
		return 0;
	lpszDst[0] = acKatakanaTable[wCode * 2U + 0U];
	lpszDst[1] = acKatakanaTable[wCode * 2U + 1U];

	return 2;
#endif
}

inline const int CAribString::PutJisKatakanaChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// JIS�J�^�J�i�����R�[�h�ϊ�
	static const TCHAR *acJisKatakanaTable = 
		TEXT("�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@")
		TEXT("�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@")
		TEXT("�@�B�u�v�A�E���@�B�D�F�H�������b")
		TEXT("�[�A�C�E�G�I�J�L�N�P�R�T�V�X�Z�\")
		TEXT("�^�`�c�e�g�i�j�k�l�m�n�q�t�w�z�}")
		TEXT("�~���������������������������J�K")
		TEXT("�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@")
		TEXT("�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@�@");

#ifdef _UNICODE
	if (dwDstLen < 1)
		return 0;
	lpszDst[0] = acJisKatakanaTable[wCode];

	return 1;
#else
	if (dwDstLen < 2)
		return 0;
	lpszDst[0] = acJisKatakanaTable[wCode * 2U + 0U];
	lpszDst[1] = acJisKatakanaTable[wCode * 2U + 1U];

	return 2;
#endif
}

inline const int CAribString::PutSymbolsChar(TCHAR *lpszDst, const DWORD dwDstLen, const WORD wCode)
{
	// �ǉ��V���{�������R�[�h�ϊ�(�Ƃ肠�����K�v�����Ȃ��̂���)
	static const LPCTSTR aszSymbolsTable1[] =
	{
		_T("[HV]"),		_T("[SD]"),		_T("[�o]"),		_T("[�v]"),		_T("[MV]"),		_T("[��]"),		_T("[��]"),		_T("[�o]"),			// 0x7A50 - 0x7A57	90/48 - 90/55
		_T("[�f]"),		_T("[�r]"),		_T("[��]"),		_T("[��]"),		_T("[��]"),		_T("[SS]"),		_T("[�a]"),		_T("[�m]"),			// 0x7A58 - 0x7A5F	90/56 - 90/63
		_T("��"),		_T("��"),		_T("[�V]"),		_T("[��]"),		_T("[�f]"),		_T("[��]"),		_T("[��]"),		_T("[�N���]"),	// 0x7A60 - 0x7A67	90/64 - 90/71
		_T("[�O]"),		_T("[��]"),		_T("[��]"),		_T("[�V]"),		_T("[��]"),		_T("[�I]"),		_T("[��]"),		_T("[��]"),			// 0x7A68 - 0x7A6F	90/72 - 90/79
		_T("[��]"),		_T("[��]"),		_T("[PPV]"),	_T("(��)"),		_T("�ق�")															// 0x7A70 - 0x7A74	90/80 - 90/84
	};

	static const LPCTSTR aszSymbolsTable2[] =
	{
		_T("��"),		_T("��"),		_T("��"),		_T("��"),		_T("��"),		_T("��"),		_T("�N"),		_T("��"),			// 0x7C21 - 0x7C28	92/01 - 92/08
		_T("��"),		_T("�~"),		_T("�u"),		_T("������"),	_T("�p"),		_T("�����p"),	_T("�����p"),	_T("�O."),			// 0x7C29 - 0x7C30	92/09 - 92/16
		_T("�P."),		_T("�Q."),		_T("�R."),		_T("�S."),		_T("�T."),		_T("�U."),		_T("�V."),		_T("�W."),			// 0x7C31 - 0x7C38	92/17 - 92/24
		_T("�X."),		_T("��"),		_T("��"),		_T("��"),		_T("��"),		_T("�O"),		_T("�V"),		_T("�O,"),			// 0x7C39 - 0x7C40	92/25 - 92/32
		_T("�P,"),		_T("�Q,"),		_T("�R,"),		_T("�S,"),		_T("�T,"),		_T("�U,"),		_T("�V,"),		_T("�W,"),			// 0x7C41 - 0x7C48	92/33 - 92/40
		_T("�X,"),		_T("(��)"),		_T("(��)"),		_T("(�L)"),		_T("(��)"),		_T("(��)"),		_T("(��)"),		_T("��"),			// 0x7C49 - 0x7C50	92/41 - 92/48
		_T("��"),		_T("�y"),		_T("�z"),		_T("��"),		_T("^2"),		_T("^3"),		_T("(CD)"),		_T("(vn)"),			// 0x7C51 - 0x7C58	92/49 - 92/56
		_T("(ob)"),		_T("(cb)"),		_T("(ce"),		_T("mb)"),		_T("(hp)"),		_T("(br)"),		_T("(p)"),		_T("(s)"),			// 0x7C59 - 0x7C60	92/57 - 92/64
		_T("(ms)"),		_T("(t)"),		_T("(bs)"),		_T("(b)"),		_T("(tb)"),		_T("(tp)"),		_T("(ds)"),		_T("(ag)"),			// 0x7C61 - 0x7C68	92/65 - 92/72
		_T("(eg)"),		_T("(vo)"),		_T("(fl)"),		_T("(ke"),		_T("y)"),		_T("(sa"),		_T("x)"),		_T("(sy"),			// 0x7C69 - 0x7C70	92/73 - 92/80
		_T("n)"),		_T("(or"),		_T("g)"),		_T("(pe"),		_T("r)"),		_T("(R)"),		_T("(C)"),		_T("(�)"),			// 0x7C71 - 0x7C78	92/81 - 92/88
		_T("DJ"),		_T("[��]"),		_T("Fax")																							// 0x7C79 - 0x7C7B	92/89 - 92/91
	};

	static const LPCTSTR aszSymbolsTable3[] =
	{
		_T("(��)"),		_T("(��)"),		_T("(��)"),		_T("(��)"),		_T("(��)"),		_T("(�y)"),		_T("(��)"),		_T("(�j)"),			// 0x7D21 - 0x7D28	93/01 - 93/08
		_T("��"),		_T("��"),		_T("��"),		_T("�~"),		_T("��"),		_T("��"),		_T("(��)"),		_T("��"),			// 0x7D29 - 0x7D30	93/09 - 93/16
		_T("�k�{�l"),	_T("�k�O�l"),	_T("�k��l"),	_T("�k���l"),	_T("�k�_�l"),	_T("�k�Łl"),	_T("�k���l"),	_T("�k���l"),		// 0x7D31 - 0x7D38	93/17 - 93/24
		_T("�k�s�l"),	_T("�k�r�l"),	_T("�m���n"),	_T("�m�߁n"),	_T("�m��n"),	_T("�m��n"),	_T("�m�O�n"),	_T("�m�V�n"),		// 0x7D39 - 0x7D40	93/25 - 93/32
		_T("�m���n"),	_T("�m���n"),	_T("�m�E�n"),	_T("�m�w�n"),	_T("�m���n"),	_T("�m�Łn"),	_T("�g"),		_T("�s"),			// 0x7D41 - 0x7D48	93/33 - 93/40
		_T("Hz"),		_T("ha"),		_T("km"),		_T("����km"),	_T("hPa"),		_T("�E"),		_T("�E"),		_T("1/2"),			// 0x7D49 - 0x7D50	93/41 - 93/48
		_T("0/3"),		_T("1/3"),		_T("2/3"),		_T("1/4"),		_T("3/4"),		_T("1/5"),		_T("2/5"),		_T("3/5"),			// 0x7D51 - 0x7D58	93/49 - 93/56
		_T("4/5"),		_T("1/6"),		_T("5/6"),		_T("1/7"),		_T("1/8"),		_T("1/9"),		_T("1/10"),		_T("����"),			// 0x7D59 - 0x7D60	93/57 - 93/64
		_T("�܂�"),		_T("�J"),		_T("��"),		_T("��"),		_T("��"),		_T("��"),		_T("��"),		_T("��"),			// 0x7D61 - 0x7D68	93/65 - 93/72
		_T("�E"),		_T("�E"),		_T("�E"),		_T("��"),		_T("��"),		_T("!!"),		_T("!?"),		_T("��/��"),		// 0x7D69 - 0x7D70	93/73 - 93/80
		_T("�J"),		_T("�J"),		_T("��"),		_T("���"),		_T("��"),		_T("���J"),		_T("�@"),		_T("�E"),			// 0x7D71 - 0x7D78	93/81 - 93/88
		_T("�E"),		_T("��"),		_T("��")																							// 0x7D79 - 0x7D7B	93/89 - 93/91
	};

	static const LPCTSTR aszSymbolsTable4[] =
	{
		_T("�T"),		_T("�U"),		_T("�V"),		_T("�W"),		_T("�X"),		_T("�Y"),		_T("�Z"),		_T("�["),			// 0x7E21 - 0x7E28	94/01 - 94/08
		_T("�\"),		_T("�]"),		_T("XI"),		_T("X�U"),		_T("�P"),		_T("�Q"),		_T("�R"),		_T("�S"),			// 0x7E29 - 0x7E30	94/09 - 94/16
		_T("(1)"),		_T("(2)"),		_T("(3)"),		_T("(4)"),		_T("(5)"),		_T("(6)"),		_T("(7)"),		_T("(8)"),			// 0x7E31 - 0x7E38	94/17 - 94/24
		_T("(9)"),		_T("(10)"),		_T("(11)"),		_T("(12)"),		_T("(21)"),		_T("(22)"),		_T("(23)"),		_T("(24)"),			// 0x7E39 - 0x7E40	94/25 - 94/32
		_T("(A)"),		_T("(B)"),		_T("(C)"),		_T("(D)"),		_T("(E)"),		_T("(F)"),		_T("(G)"),		_T("(H)"),			// 0x7E41 - 0x7E48	94/33 - 94/40
		_T("(I)"),		_T("(J)"),		_T("(K)"),		_T("(L)"),		_T("(M)"),		_T("(N)"),		_T("(O)"),		_T("(P)"),			// 0x7E49 - 0x7E50	94/41 - 94/48
		_T("(Q)"),		_T("(R)"),		_T("(S)"),		_T("(T)"),		_T("(U)"),		_T("(V)"),		_T("(W)"),		_T("(X)"),			// 0x7E51 - 0x7E58	94/49 - 94/56
		_T("(Y)"),		_T("(Z)"),		_T("(25)"),		_T("(26)"),		_T("(27)"),		_T("(28)"),		_T("(29)"),		_T("(30)"),			// 0x7E59 - 0x7E60	94/57 - 94/64
		_T("�@"),		_T("�A"),		_T("�B"),		_T("�C"),		_T("�D"),		_T("�E"),		_T("�F"),		_T("�G"),			// 0x7E61 - 0x7E68	94/65 - 94/72
		_T("�H"),		_T("�I"),		_T("�J"),		_T("�K"),		_T("�L"),		_T("�M"),		_T("�N"),		_T("�O"),			// 0x7E69 - 0x7E70	94/73 - 94/80
		_T("�@"),		_T("�A"),		_T("�B"),		_T("�C"),		_T("�D"),		_T("�E"),		_T("�F"),		_T("�G"),			// 0x7E71 - 0x7E78	94/81 - 94/88
		_T("�H"),		_T("�I"),		_T("�J"),		_T("�K"),		_T("(31)")															// 0x7E79 - 0x7E7D	94/89 - 94/93
	};

	// �V���{����ϊ�����
	LPCTSTR pszSrc;
	if ((wCode >= 0x7A50U) && (wCode <= 0x7A74U)) {
		pszSrc = aszSymbolsTable1[wCode - 0x7A50U];
	} else if ((wCode >= 0x7C21U) && (wCode <= 0x7C7BU)) {
		pszSrc = aszSymbolsTable2[wCode - 0x7C21U];
	} else if((wCode >= 0x7D21U) && (wCode <= 0x7D7BU)) {
		pszSrc = aszSymbolsTable3[wCode - 0x7D21U];
	} else if((wCode >= 0x7E21U) && (wCode <= 0x7E7DU)) {
		pszSrc = aszSymbolsTable4[wCode - 0x7E21U];
	} else {
		pszSrc = TEXT("�E");
	}
	DWORD Length = ::lstrlen(pszSrc);
	if (dwDstLen < Length)
		return 0;
	::CopyMemory(lpszDst, pszSrc, Length * sizeof(TCHAR));

	return Length;
}

inline void CAribString::ProcessEscapeSeq(const BYTE byCode)
{
	// �G�X�P�[�v�V�[�P���X����
	switch(m_byEscSeqCount){
		// 1�o�C�g��
		case 1U	:
			switch(byCode){
				// Invocation of code elements
				case 0x6EU	: LockingShiftGL(2U);	m_byEscSeqCount = 0U;	return;		// LS2
				case 0x6FU	: LockingShiftGL(3U);	m_byEscSeqCount = 0U;	return;		// LS3
				case 0x7EU	: LockingShiftGR(1U);	m_byEscSeqCount = 0U;	return;		// LS1R
				case 0x7DU	: LockingShiftGR(2U);	m_byEscSeqCount = 0U;	return;		// LS2R
				case 0x7CU	: LockingShiftGR(3U);	m_byEscSeqCount = 0U;	return;		// LS3R

				// Designation of graphic sets
				case 0x24U	:	
				case 0x28U	: m_byEscSeqIndex = 0U;		break;
				case 0x29U	: m_byEscSeqIndex = 1U;		break;
				case 0x2AU	: m_byEscSeqIndex = 2U;		break;
				case 0x2BU	: m_byEscSeqIndex = 3U;		break;
				default		: m_byEscSeqCount = 0U;		return;		// �G���[
				}
			break;

		// 2�o�C�g��
		case 2U	:
			if(DesignationGSET(m_byEscSeqIndex, byCode)){
				m_byEscSeqCount = 0U;
				return;
				}
			
			switch(byCode){
				case 0x20	: m_bIsEscSeqDrcs = true;	break;
				case 0x28	: m_bIsEscSeqDrcs = true;	m_byEscSeqIndex = 0U;	break;
				case 0x29	: m_bIsEscSeqDrcs = false;	m_byEscSeqIndex = 1U;	break;
				case 0x2A	: m_bIsEscSeqDrcs = false;	m_byEscSeqIndex = 2U;	break;
				case 0x2B	: m_bIsEscSeqDrcs = false;	m_byEscSeqIndex = 3U;	break;
				default		: m_byEscSeqCount = 0U;		return;		// �G���[
				}
			break;

		// 3�o�C�g��
		case 3U	:
			if(!m_bIsEscSeqDrcs){
				if(DesignationGSET(m_byEscSeqIndex, byCode)){
					m_byEscSeqCount = 0U;
					return;
					}
				}
			else{
				if(DesignationDRCS(m_byEscSeqIndex, byCode)){
					m_byEscSeqCount = 0U;
					return;
					}
				}

			if(byCode == 0x20U){
				m_bIsEscSeqDrcs = true;
				}
			else{
				// �G���[
				m_byEscSeqCount = 0U;
				return;
				}
			break;

		// 4�o�C�g��
		case 4U	:
			DesignationDRCS(m_byEscSeqIndex, byCode);
			m_byEscSeqCount = 0U;
			return;
		}

	m_byEscSeqCount++;
}

inline void CAribString::LockingShiftGL(const BYTE byIndexG)
{
	// LSx
	m_pLockingGL = &m_CodeG[byIndexG];
}

inline void CAribString::LockingShiftGR(const BYTE byIndexG)
{
	// LSxR
	m_pLockingGR = &m_CodeG[byIndexG];
}

inline void CAribString::SingleShiftGL(const BYTE byIndexG)
{
	// SSx
	m_pSingleGL  = &m_CodeG[byIndexG];
}

inline const bool CAribString::DesignationGSET(const BYTE byIndexG, const BYTE byCode)
{
	// G�̃O���t�B�b�N�Z�b�g�����蓖�Ă�
	switch(byCode){
		case 0x42U	: m_CodeG[byIndexG] = CODE_KANJI;				return true;	// Kanji
		case 0x4AU	: m_CodeG[byIndexG] = CODE_ALPHANUMERIC;		return true;	// Alphanumeric
		case 0x30U	: m_CodeG[byIndexG] = CODE_HIRAGANA;			return true;	// Hiragana
		case 0x31U	: m_CodeG[byIndexG] = CODE_KATAKANA;			return true;	// Katakana
		case 0x32U	: m_CodeG[byIndexG] = CODE_MOSAIC_A;			return true;	// Mosaic A
		case 0x33U	: m_CodeG[byIndexG] = CODE_MOSAIC_B;			return true;	// Mosaic B
		case 0x34U	: m_CodeG[byIndexG] = CODE_MOSAIC_C;			return true;	// Mosaic C
		case 0x35U	: m_CodeG[byIndexG] = CODE_MOSAIC_D;			return true;	// Mosaic D
		case 0x36U	: m_CodeG[byIndexG] = CODE_PROP_ALPHANUMERIC;	return true;	// Proportional Alphanumeric
		case 0x37U	: m_CodeG[byIndexG] = CODE_PROP_HIRAGANA;		return true;	// Proportional Hiragana
		case 0x38U	: m_CodeG[byIndexG] = CODE_PROP_KATAKANA;		return true;	// Proportional Katakana
		case 0x49U	: m_CodeG[byIndexG] = CODE_JIS_X0201_KATAKANA;	return true;	// JIS X 0201 Katakana
		case 0x39U	: m_CodeG[byIndexG] = CODE_JIS_KANJI_PLANE_1;	return true;	// JIS compatible Kanji Plane 1
		case 0x3AU	: m_CodeG[byIndexG] = CODE_JIS_KANJI_PLANE_2;	return true;	// JIS compatible Kanji Plane 2
		case 0x3BU	: m_CodeG[byIndexG] = CODE_ADDITIONAL_SYMBOLS;	return true;	// Additional symbols
		default		: return false;		// �s���ȃO���t�B�b�N�Z�b�g
		}
}

inline const bool CAribString::DesignationDRCS(const BYTE byIndexG, const BYTE byCode)
{
	// DRCS�̃O���t�B�b�N�Z�b�g�����蓖�Ă�
	switch(byCode){
		case 0x40U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-0
		case 0x41U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-1
		case 0x42U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-2
		case 0x43U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-3
		case 0x44U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-4
		case 0x45U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-5
		case 0x46U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-6
		case 0x47U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-7
		case 0x48U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-8
		case 0x49U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-9
		case 0x4AU	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-10
		case 0x4BU	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-11
		case 0x4CU	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-12
		case 0x4DU	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-13
		case 0x4EU	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-14
		case 0x4FU	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// DRCS-15
		case 0x70U	: m_CodeG[byIndexG] = CODE_UNKNOWN;				return true;	// Macro
		default		: return false;		// �s���ȃO���t�B�b�N�Z�b�g
		}
}


/////////////////////////////////////////////////////////////////////////////
// ARIB STD-B10 Part2 Annex C MJD+JTC �����N���X
/////////////////////////////////////////////////////////////////////////////

const bool CAribTime::AribToSystemTime(const BYTE *pHexData, SYSTEMTIME *pSysTime)
{
	// �S�r�b�g��1�̂Ƃ��͖���`
	if((*((DWORD *)pHexData) == 0xFFFFFFFFUL) && (pHexData[4] == 0xFFU))return false;

	// MJD�`���̓��t�����
	SplitAribMjd(((WORD)pHexData[0] << 8) | (WORD)pHexData[1], &pSysTime->wYear, &pSysTime->wMonth, &pSysTime->wDay, &pSysTime->wDayOfWeek);

	// BCD�`���̎��������
	SplitAribBcd(&pHexData[2], &pSysTime->wHour, &pSysTime->wMinute, &pSysTime->wSecond);

	// �~���b�͏��0
	pSysTime->wMilliseconds = 0U;

	return true;
}

void CAribTime::SplitAribMjd(const WORD wAribMjd, WORD *pwYear, WORD *pwMonth, WORD *pwDay, WORD *pwDayOfWeek)
{
	// MJD�`���̓��t����͂���
	const DWORD dwYd = (DWORD)(((double)wAribMjd - 15078.2) / 365.25);
	const DWORD dwMd = (DWORD)(((double)wAribMjd - 14956.1 - (double)((int)((double)dwYd * 365.25))) / 30.6001);
	const DWORD dwK = ((dwMd == 14UL) || (dwMd == 15UL))? 1U : 0U;

	if(pwDay)*pwDay = wAribMjd - 14956U - (WORD)((double)dwYd * 365.25) - (WORD)((double)dwMd * 30.6001);
	if(pwYear)*pwYear = (WORD)(dwYd + dwK) + 1900U;
	if(pwMonth)*pwMonth	= (WORD)(dwMd - 1UL - dwK * 12UL);
	if(pwDayOfWeek)*pwDayOfWeek = (wAribMjd + 2U) % 7U;
}

void CAribTime::SplitAribBcd(const BYTE *pAribBcd, WORD *pwHour, WORD *pwMinute, WORD *pwSecond)
{
	// BCD�`���̎�������͂���
	if(pwHour)*pwHour		= (WORD)(pAribBcd[0] >> 4) * 10U + (WORD)(pAribBcd[0] & 0x0FU);
	if(pwMinute)*pwMinute	= (WORD)(pAribBcd[1] >> 4) * 10U + (WORD)(pAribBcd[1] & 0x0FU);
	if(pwSecond)*pwSecond	= (WORD)(pAribBcd[2] >> 4) * 10U + (WORD)(pAribBcd[2] & 0x0FU);
}

const DWORD CAribTime::AribBcdToSecond(const BYTE *pAribBcd)
{
	// �S�r�b�g��1�̂Ƃ��͖���`
	if (pAribBcd[0] == 0xFF && pAribBcd[1] == 0xFF && pAribBcd[2] == 0xFF)
		return 0;

	// BCD�`���̎�����b�ɕϊ�����
	const DWORD dwSecond = (((DWORD)(pAribBcd[0] >> 4) * 10U + (DWORD)(pAribBcd[0] & 0x0FU)) * 3600UL)
						 + (((DWORD)(pAribBcd[1] >> 4) * 10U + (DWORD)(pAribBcd[1] & 0x0FU)) * 60UL)
						 + ((DWORD)(pAribBcd[2] >> 4) * 10U + (DWORD)(pAribBcd[2] & 0x0FU));

	return dwSecond;
}
