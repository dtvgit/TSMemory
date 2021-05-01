#include <windows.h>
#include <tchar.h>
#include "Image.h"




SIZE_T CalcDIBInfoSize(const BITMAPINFOHEADER *pbmih)
{
	SIZE_T Size;

	Size=sizeof(BITMAPINFOHEADER);
	if (pbmih->biBitCount<=8)
		Size+=(1<<pbmih->biBitCount)*sizeof(RGBQUAD);
	else if (pbmih->biCompression==BI_BITFIELDS)
		Size+=3*sizeof(DWORD);
	return Size;
}


SIZE_T CalcDIBBitsSize(const BITMAPINFOHEADER *pbmih)
{
	return DIB_ROW_BYTES(pbmih->biWidth,pbmih->biBitCount)*abs(pbmih->biHeight);
}


SIZE_T CalcDIBSize(const BITMAPINFOHEADER *pbmih)
{
	return CalcDIBInfoSize(pbmih)+CalcDIBBitsSize(pbmih);
}




CImageCodec::CImageCodec()
{
	m_hLib=NULL;
}


CImageCodec::~CImageCodec()
{
	if (m_hLib==NULL)
		FreeLibrary(m_hLib);
}


bool CImageCodec::Init()
{
	if (m_hLib==NULL) {
		m_hLib=LoadLibrary(TEXT("TVTest_Image.dll"));
		if (m_hLib==NULL)
			return false;
		m_pSaveImage=(SaveImageFunc)GetProcAddress(m_hLib,"SaveImage");
		if (m_pSaveImage==NULL) {
			FreeLibrary(m_hLib);
			m_hLib=NULL;
			return false;
		}
	}
	return true;
}


bool CImageCodec::SaveImage(LPCTSTR pszFileName,int Format,LPCTSTR pszOption,
				const BITMAPINFO *pbmi,const void *pBits,LPCTSTR pszComment)
{
	ImageSaveInfo Info;

	if (m_hLib==NULL && !Init())
		return false;
#ifdef UNICODE
	Info.pszFileName=pszFileName;
	Info.pszFormat=EnumSaveFormat(Format);
	Info.pszOption=pszOption;
	Info.pbmi=pbmi;
	Info.pBits=pBits;
	Info.pszComment=pszComment;
#else
	WCHAR szFileName[MAX_PATH],szFormat[8],szOption[32],szComment[256];

	MultiByteToWideChar(CP_ACP,0,pszFileName,-1,szFileName,MAX_PATH);
	MultiByteToWideChar(CP_ACP,0,EnumSaveFormat(Format),-1,szFormat,8);
	MultiByteToWideChar(CP_ACP,0,pszOption,-1,szOption,32);
	MultiByteToWideChar(CP_ACP,0,pszComment,-1,szComment,256);
	Info.pszFileName=szFileName;
	Info.pszFormat=szFormat;
	Info.pszOption=szOption;
	Info.pbmi=pbmi;
	Info.pBits=pBits;
	Info.pszComment=szComment;
#endif
	return m_pSaveImage(&Info)==TRUE;
}


LPCTSTR CImageCodec::EnumSaveFormat(int Index) const
{
	switch (Index) {
	case 0:	return TEXT("BMP");
	case 1:	return TEXT("JPEG");
	case 2:	return TEXT("PNG");
	}
	return NULL;
}


LPCTSTR CImageCodec::GetExtension(int Index) const
{
	switch (Index) {
	case 0:	return TEXT("bmp");
	case 1:	return TEXT("jpg");
	case 2:	return TEXT("png");
	}
	return NULL;
}


int CImageCodec::FormatNameToIndex(LPCTSTR pszName) const
{
	int i;
	LPCTSTR pszFormat;

	for (i=0;(pszFormat=EnumSaveFormat(i))!=NULL;i++) {
		if (lstrcmpi(pszName,pszFormat)==0)
			return i;
	}
	return -1;
}
