#include <windows.h>
#include <windowsx.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <tchar.h>
#include "filter.h"
#include "Image.h"


#pragma comment(lib,"shlwapi.lib")


#define numberof(a) (sizeof(a)/sizeof(a[0]))

#define WINDOW_WIDTH	256
#define WINDOW_HEIGHT	40

#define IDC_FILENAME	1000
#define IDC_FORMAT		1001
#define IDC_JPEGLEVEL	1002
#define IDC_PNGLEVEL	1003
#define IDC_CAPTURE		1004


static HINSTANCE hInst;
static CImageCodec ImageCodec;
static HFONT hfontControls=NULL;
static HWND hwndFileName=NULL;
static HWND hwndFormat=NULL;
static HWND hwndJpegLevel=NULL;
static HWND hwndPngLevel=NULL;
static HWND hwndCapture=NULL;

static TCHAR szIniFileName[MAX_PATH];
static TCHAR szSaveFileName[MAX_PATH];
static TCHAR szSaveFormat[8]=TEXT("bmp");
static int JpegLevel=90;
static int PngLevel=6;
static bool fCopyFileName=false;




static FILTER_DLL filter = {
	FILTER_FLAG_ALWAYS_ACTIVE | FILTER_FLAG_MAIN_MESSAGE | FILTER_FLAG_WINDOW_SIZE | FILTER_FLAG_DISP_FILTER | FILTER_FLAG_EX_INFORMATION,
	FILTER_WINDOW_SIZE_CLIENT | WINDOW_WIDTH,
	FILTER_WINDOW_SIZE_CLIENT | WINDOW_HEIGHT,
	"キャプチャ・ユーティリティ",
	0, NULL, NULL, NULL, NULL,
	0, NULL, NULL,
	NULL,
	func_init,
	func_exit,
	NULL,
	func_WndProc,
	NULL, NULL,
	NULL, 0,
	"キャプチャ・ユーティリティ ver.0.2.0",
	NULL, NULL,
	NULL,
	NULL, NULL,
	NULL,
	NULL, NULL, NULL, NULL,
	NULL,
	{0,0}
};


BOOL WINAPI DllMain(HINSTANCE hInstance,DWORD dwReason,LPVOID pvReserved)
{
	if (dwReason==DLL_PROCESS_ATTACH)
		hInst=hInstance;
	return TRUE;
}


EXTERN_C __declspec(dllexport) FILTER_DLL * __stdcall GetFilterTable(void)
{
	return &filter;
}


BOOL func_init(FILTER *fp)
{
	GetModuleFileName(hInst,szIniFileName,MAX_PATH);
	PathRenameExtension(szIniFileName,TEXT(".ini"));
	GetPrivateProfileString(TEXT("Settings"),TEXT("FileName"),NULL,szSaveFileName,numberof(szSaveFileName),szIniFileName);
	if (szSaveFileName[0]=='\0') {
		SHGetSpecialFolderPath(NULL,szSaveFileName,CSIDL_MYPICTURES,FALSE);
		PathAppend(szSaveFileName,TEXT("Capture"));
	}
	GetPrivateProfileString(TEXT("Settings"),TEXT("Format"),TEXT("bmp"),szSaveFormat,numberof(szSaveFormat),szIniFileName);
	JpegLevel=GetPrivateProfileInt(TEXT("Settings"),TEXT("JpegLevel"),JpegLevel,szIniFileName);
	PngLevel=GetPrivateProfileInt(TEXT("Settings"),TEXT("PngLevel"),PngLevel,szIniFileName);
	fCopyFileName=GetPrivateProfileInt(TEXT("Settings"),TEXT("CopyFileName"),fCopyFileName,szIniFileName)!=0;

	ImageCodec.Init();
	return TRUE;
}


static BOOL WritePrivateProfileInt(LPCTSTR pszSection,LPCTSTR pszKey,
								   int Value,LPCTSTR pszFile)
{
	TCHAR szValue[16];

	wsprintf(szValue,TEXT("%d"),Value);
	return WritePrivateProfileString(pszSection,pszKey,szValue,pszFile);
}


BOOL func_exit(FILTER *fp)
{
	WritePrivateProfileString(TEXT("Settings"),TEXT("FileName"),szSaveFileName,szIniFileName);
	WritePrivateProfileString(TEXT("Settings"),TEXT("Format"),szSaveFormat,szIniFileName);
	WritePrivateProfileInt(TEXT("Settings"),TEXT("JpegLevel"),JpegLevel,szIniFileName);
	WritePrivateProfileInt(TEXT("Settings"),TEXT("PngLevel"),PngLevel,szIniFileName);
	//WritePrivateProfileInt(TEXT("Settings"),TEXT("CopyFileName"),fCopyFileName,szIniFileName);

	if (hfontControls!=NULL) {
		DeleteObject(hfontControls);
		hfontControls=NULL;
	}

	return TRUE;
}


BOOL func_WndProc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam,void *editp,FILTER *fp)
{
	switch (message) {
	case WM_FILTER_INIT:
		{
			hfontControls=CreateFont(-12,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,
								   DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,FF_DONTCARE,TEXT("ＭＳ Ｐゴシック"));

			RECT rc;
			GetClientRect(hwnd,&rc);
			hwndFileName=CreateWindowEx(0,TEXT("EDIT"),TEXT(""),
										WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL | ES_NOHIDESEL,
										0,0,rc.right,rc.bottom/2,hwnd,
										(HMENU)IDC_FILENAME,hInst,NULL);
			SetWindowFont(hwndFileName,hfontControls,TRUE);
			Edit_LimitText(hwndFileName,MAX_PATH-1);
			SetWindowText(hwndFileName,szSaveFileName);
			hwndFormat=CreateWindowEx(0,TEXT("COMBOBOX"),TEXT(""),
									  WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
									  0,rc.bottom/2,64,128,hwnd,
									  (HMENU)IDC_FORMAT,hInst,NULL);
			SetWindowFont(hwndFormat,hfontControls,TRUE);
			for (int i=0;;i++) {
				LPCTSTR pszFormat=ImageCodec.EnumSaveFormat(i);

				if (pszFormat==NULL)
					break;
				ComboBox_AddString(hwndFormat,pszFormat);
			}
			ComboBox_SetCurSel(hwndFormat,ImageCodec.FormatNameToIndex(szSaveFormat));
			hwndJpegLevel=CreateWindowEx(0,TEXT("COMBOBOX"),TEXT(""),
										 WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWN,
										 64,rc.bottom/2,48,128,hwnd,
										 (HMENU)IDC_JPEGLEVEL,hInst,NULL);
			SetWindowFont(hwndJpegLevel,hfontControls,TRUE);
			for (int i=1;i<=10;i++) {
				TCHAR szText[4];

				wsprintf(szText,TEXT("%d"),i*10);
				ComboBox_AddString(hwndJpegLevel,szText);
			}
			SetDlgItemInt(hwnd,IDC_JPEGLEVEL,JpegLevel,TRUE);
			EnableWindow(hwndJpegLevel,lstrcmpi(szSaveFormat,TEXT("jpeg"))==0);
			hwndPngLevel=CreateWindowEx(0,TEXT("COMBOBOX"),TEXT(""),
										WS_CHILD | WS_VISIBLE | WS_VSCROLL | CBS_DROPDOWNLIST,
										112,rc.bottom/2,48,128,hwnd,
										(HMENU)IDC_PNGLEVEL,hInst,NULL);
			SetWindowFont(hwndPngLevel,hfontControls,TRUE);
			for (int i=0;i<=9;i++) {
				TCHAR szText[2];

				wsprintf(szText,TEXT("%d"),i);
				ComboBox_AddString(hwndPngLevel,szText);
			}
			ComboBox_SetCurSel(hwndPngLevel,PngLevel);
			EnableWindow(hwndPngLevel,lstrcmpi(szSaveFormat,TEXT("png"))==0);
			hwndCapture=CreateWindowEx(0,TEXT("BUTTON"),TEXT("保存"),
									   WS_CHILD | WS_VISIBLE,
									   160,rc.bottom/2,96,rc.bottom/2,hwnd,
									   (HMENU)IDC_CAPTURE,hInst,NULL);
			SetWindowFont(hwndCapture,hfontControls,TRUE);
		}
		break;

	case WM_FILTER_EXIT:
		{
			GetWindowText(hwndFileName,szSaveFileName,numberof(szSaveFileName));

			int Format=ComboBox_GetCurSel(hwndFormat);
			LPCTSTR pszFormatName=ImageCodec.EnumSaveFormat(Format);
			if (pszFormatName!=NULL)
				lstrcpy(szSaveFormat,pszFormatName);

			JpegLevel=GetDlgItemInt(hwnd,IDC_JPEGLEVEL,NULL,TRUE);
			PngLevel=ComboBox_GetCurSel(hwndPngLevel);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wparam)) {
		case IDC_CAPTURE:
			if (fp->exfunc->is_editing(editp)) {
				int Format=ComboBox_GetCurSel(hwndFormat);
				LPCTSTR pszFormatName=ImageCodec.EnumSaveFormat(Format);
				TCHAR szFileName[MAX_PATH],szOption[16];

				if (pszFormatName==NULL)
					break;
				GetWindowText(hwndFileName,szFileName,MAX_PATH);
				if (szFileName[0]=='\0') {
					MessageBox(hwnd,TEXT("ファイル名を入力してください。"),NULL,MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				LPTSTR pszExtension=PathFindExtension(szFileName);
				if (*pszExtension=='\0')
					wsprintf(pszExtension,TEXT(".%s"),ImageCodec.GetExtension(Format));
				if (::PathFileExists(szFileName)) {
					TCHAR szPath[MAX_PATH];
					int ExtensionOffset=pszExtension-szFileName;

					::lstrcpy(szPath,szFileName);
					for (int i=0;;i++) {
						::wsprintf(szPath+ExtensionOffset,TEXT("%d%s"),i+1,pszExtension);
						if (!::PathFileExists(szPath)) {
							::lstrcpy(szFileName,szPath);
							break;
						}
					}
				}

				if (lstrcmpi(pszFormatName,"jpeg")==0) {
					wsprintf(szOption,TEXT("%d"),GetDlgItemInt(hwnd,IDC_JPEGLEVEL,NULL,TRUE));
				} else if (lstrcmpi(pszFormatName,"png")==0) {
					wsprintf(szOption,TEXT("%d"),ComboBox_GetCurSel(hwndPngLevel));
				} else {
					szOption[0]='\0';
				}

				SYS_INFO si;
				BYTE *pBuffer;
				int Width,Height;
				fp->exfunc->get_sys_info(editp,&si);
				pBuffer=new BYTE[si.vram_w*si.vram_h*3];
				if (!fp->exfunc->get_pixel_filtered_ex(editp,fp->exfunc->get_frame(editp),pBuffer,&Width,&Height,NULL)) {
					delete [] pBuffer;
					MessageBox(hwnd,TEXT("画像を取得できません。"),NULL,MB_OK | MB_ICONEXCLAMATION);
					break;
				}
				HCURSOR hcurOld=SetCursor(LoadCursor(NULL,IDC_WAIT));
				BITMAPINFO bmi;
				bmi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
				bmi.bmiHeader.biWidth=Width;
				bmi.bmiHeader.biHeight=Height;
				bmi.bmiHeader.biPlanes=1;
				bmi.bmiHeader.biBitCount=24;
				bmi.bmiHeader.biCompression=BI_RGB;
				bmi.bmiHeader.biSizeImage=0;
				bmi.bmiHeader.biXPelsPerMeter=0;
				bmi.bmiHeader.biYPelsPerMeter=0;
				bmi.bmiHeader.biClrUsed=0;
				bmi.bmiHeader.biClrImportant=0;
				bool fResult=ImageCodec.SaveImage(szFileName,Format,szOption,
												  &bmi,pBuffer,NULL);
				delete [] pBuffer;
				SetCursor(hcurOld);
				if (!fResult) {
					MessageBox(hwnd,TEXT("保存時にエラーが発生しました。"),NULL,MB_OK | MB_ICONEXCLAMATION);
				}
				if (fCopyFileName) {
					if (OpenClipboard(hwnd)) {
						HGLOBAL hGlobal;

						EmptyClipboard();
						hGlobal=GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE,
											(::lstrlen(szFileName)+1)*sizeof(TCHAR));
						if (hGlobal!=NULL) {
							lstrcpy((LPTSTR)GlobalLock(hGlobal),szFileName);
							GlobalUnlock(hGlobal);
							SetClipboardData(
#ifdef UNICODE
								CF_UNICODETEXT,
#else
								CF_TEXT,
#endif
								hGlobal);
						}
						CloseClipboard();
					}
				}
			}
			break;

		case IDC_FORMAT:
			if (HIWORD(wparam)==CBN_SELCHANGE) {
				int Format=ComboBox_GetCurSel(hwndFormat);
				LPCTSTR pszFormatName=ImageCodec.EnumSaveFormat(Format);

				if (pszFormatName!=NULL)
					lstrcpy(szSaveFormat,pszFormatName);
				EnableWindow(hwndJpegLevel,
					pszFormatName!=NULL && lstrcmpi(pszFormatName,TEXT("jpeg"))==0);
				EnableWindow(hwndPngLevel,
					pszFormatName!=NULL && lstrcmpi(pszFormatName,TEXT("png"))==0);
			}
			break;

		/*
		case IDC_JPEGLEVEL:
			if (HIWORD(wparam)==CBN_SELCHANGE || HIWORD(wparam)==CBN_EDITCHANGE) {
				JpegLevel=GetDlgItemInt(hwnd,IDC_JPEGLEVEL,NULL,TRUE);
			}
			break;

		case IDC_PNGLEVEL:
			if (HIWORD(wparam)==CBN_SELCHANGE) {
				PngLevel=ComboBox_GetCurSel(hwndPngLevel);
			}
			break;
		*/
		}
		break;
	}
	return FALSE;
}
