#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <tchar.h>
#define TVTEST_PLUGIN_CLASS_IMPLEMENT
#include "TVTestPlugin.h"
#include "BonTsEngine/TsSelector.h"


#pragma comment(lib,"shlwapi.lib")


#define TS_MEMORY_WINDOW_CLASS TEXT("TSMemory Window")

#define DEFAULT_BUFFER_LENGTH (1024*1024*10/188)

#include <pshpack1.h>

struct BufferInfo {
	DWORD Size;
	DWORD Used;
	DWORD Pos;
	DWORD Reserved;
};

#include <poppack.h>


// プラグインクラス
class CTSMemory : public TVTest::CTVTestPlugin , public CMediaDecoder
{
	class CMutexLock
	{
		HANDLE m_hMutex;
	public:
		CMutexLock() : m_hMutex(NULL) {}
		~CMutexLock() { Close(); }
		bool Create(LPCTSTR pszName) {
			if (m_hMutex!=NULL)
				return false;

			SECURITY_DESCRIPTOR sd;
			SECURITY_ATTRIBUTES sa;
			::ZeroMemory(&sd,sizeof(sd));
			::InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
			::SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE);
			::ZeroMemory(&sa,sizeof(sa));
			sa.nLength=sizeof(sa);
			sa.lpSecurityDescriptor=&sd;

			m_hMutex=::CreateMutex(&sa,FALSE,pszName);
			if (m_hMutex==NULL)
				return false;
			return true;
		}
		void Close() {
			if (m_hMutex!=NULL) {
				::CloseHandle(m_hMutex);
				m_hMutex=NULL;
			}
		}
		bool Lock() {
			if (m_hMutex==NULL)
				return false;
			DWORD Result=::WaitForSingleObject(m_hMutex,2000);
			if (Result!=WAIT_OBJECT_0 && Result!=WAIT_ABANDONED) {
				Close();
				return false;
			}
			return true;
		}
		void Unlock() {
			if (m_hMutex!=NULL)
				::ReleaseMutex(m_hMutex);
		}
	};

	HWND m_hwnd;
	HANDLE m_hMutex;
	BYTE *m_pBuffer;
	SIZE_T m_BufferSize;
	SIZE_T m_BufferPos;
	SIZE_T m_BufferUsed;
	HANDLE m_hFileMapping;
	CMutexLock m_BufferLock;
	CTsSelector m_TsSelector;
	CTsPacket m_TsPacket;
	BYTE m_ContCounter[0x1FFF];
	TCHAR m_szIniFileName[MAX_PATH];
	TCHAR m_szAviUtlPath[MAX_PATH];
	bool m_fActivateAviUtl;
	bool m_fCloseAviUtl;
	TCHAR m_szFileName[MAX_PATH];
	TCHAR m_szMutexName[MAX_PATH];
	DWORD m_AviUtlProcessID;
	void FreeBuffer();
	void PurgeBuffer();
	HWND FindAviUtlWindow();
	static LRESULT CALLBACK EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData);
	static BOOL CALLBACK StreamCallback(BYTE *pData,void *pClientData);
	static CTSMemory *GetThis(HWND hwnd);
	static LRESULT CALLBACK WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
public:
	CTSMemory();
	~CTSMemory();
	virtual bool GetPluginInfo(TVTest::PluginInfo *pInfo);
	virtual bool Initialize();
	virtual bool Finalize();

// CMediaDecoder
	virtual const bool InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex = 0UL);
};


CTSMemory::CTSMemory()
	: m_hwnd(NULL)
	, m_hMutex(NULL)
	, m_pBuffer(NULL)
	, m_BufferSize(DEFAULT_BUFFER_LENGTH)
	, m_hFileMapping(NULL)
	, m_AviUtlProcessID(0)
{
}


CTSMemory::~CTSMemory()
{
}


bool CTSMemory::GetPluginInfo(TVTest::PluginInfo *pInfo)
{
	// プラグインの情報を返す
	pInfo->Type           = TVTest::PLUGIN_TYPE_NORMAL;
	pInfo->Flags          = TVTest::PLUGIN_FLAG_HASSETTINGS;
	pInfo->pszPluginName  = L"TSMemory";
	pInfo->pszCopyright   = L"Public Domain";
	pInfo->pszDescription = L"映像メモリ";
	return true;
}


bool CTSMemory::Initialize()
{
	// 初期化処理

	// 設定の読み込み
	::GetModuleFileName(g_hinstDLL,m_szIniFileName,MAX_PATH);
	::PathRenameExtension(m_szIniFileName,TEXT(".ini"));
	int MemorySize=::GetPrivateProfileInt(TEXT("Settings"),TEXT("MemorySize"),10,m_szIniFileName);
	if (MemorySize<0)
		MemorySize=1;
	m_BufferSize=MemorySize*(1024*1024)/188;
	::GetPrivateProfileString(TEXT("Settings"),TEXT("AviUtlPath"),NULL,m_szAviUtlPath,MAX_PATH,m_szIniFileName);
	if (m_szAviUtlPath[0]=='\0') {
		m_pApp->AddLog(TEXT("AviUtlのパスが設定させていません。"));
		return false;
	}
	m_fActivateAviUtl=::GetPrivateProfileInt(TEXT("Settings"),TEXT("Activate"),1,m_szIniFileName)!=0;
	m_fCloseAviUtl=::GetPrivateProfileInt(TEXT("Settings"),TEXT("AutoClose"),0,m_szIniFileName)!=0;

	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	::ZeroMemory(&sd,sizeof(sd));
	::InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
	::SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE);
	::ZeroMemory(&sa,sizeof(sa));
	sa.nLength=sizeof(sa);
	sa.lpSecurityDescriptor=&sd;
	for (int i=0;i<100;i++) {
		TCHAR szName[64];

		::wsprintf(szName,TEXT("tsmemory%d.tvtv.mutex"),i);
		m_hMutex=::CreateMutex(&sa,FALSE,szName);
		if (m_hMutex!=NULL) {
			if (::GetLastError()!=ERROR_ALREADY_EXISTS) {
				::lstrcpy(m_szMutexName,szName);
				::GetModuleFileName(g_hinstDLL,m_szFileName,MAX_PATH);
				::wsprintf(::PathFindFileName(m_szFileName),TEXT("tsmemory%d.tvtv"),i);
				break;
			}
			::CloseHandle(m_hMutex);
		}
		if (i==99) {
			m_hMutex=NULL;
			m_pApp->AddLog(TEXT("Mutexを作成できません。"));
			return false;
		}
	}

	m_BufferLock.Create(m_szMutexName);

	m_TsPacket.GetBuffer(188);
	::FillMemory(m_ContCounter,sizeof(m_ContCounter),0x10);

	m_TsSelector.SetOutputDecoder(this);
	m_TsSelector.SetTargetServiceID(0,CTsSelector::STREAM_MPEG2VIDEO);

	// ウィンドウクラスの登録
	WNDCLASS wc;
	wc.style=0;
	wc.lpfnWndProc=WndProc;
	wc.cbClsExtra=0;
	wc.cbWndExtra=0;
	wc.hInstance=g_hinstDLL;
	wc.hIcon=NULL;
	wc.hCursor=NULL;
	wc.hbrBackground=NULL;
	wc.lpszMenuName=NULL;
	wc.lpszClassName=TS_MEMORY_WINDOW_CLASS;
	if (::RegisterClass(&wc)==0)
		return false;

	// ウィンドウの作成
	if (::CreateWindowEx(0,TS_MEMORY_WINDOW_CLASS,NULL,WS_POPUP,
							0,0,0,0,
							m_pApp->GetAppWindow(),NULL,g_hinstDLL,this)==NULL)
		return false;

	// コマンドを登録
	m_pApp->RegisterCommand(1, L"Execute", L"実行");

	// イベントコールバック関数を登録
	m_pApp->SetEventCallback(EventCallback,this);

	// ストリームコールバック関数を登録
	m_pApp->SetStreamCallback(0,StreamCallback,this);

	return true;
}


void CTSMemory::FreeBuffer()
{
	if (m_BufferLock.Lock()) {
		if (m_pBuffer!=NULL) {
			::UnmapViewOfFile(m_pBuffer);
			m_pBuffer=NULL;
			::CloseHandle(m_hFileMapping);
			m_hFileMapping=NULL;
		}
		m_BufferLock.Unlock();
	}
}


void CTSMemory::PurgeBuffer()
{
	if (m_BufferLock.Lock()) {
		m_BufferUsed=0;
		m_BufferPos=0;
		if (m_pBuffer!=NULL) {
			BufferInfo *pInfo=(BufferInfo*)m_pBuffer;

			pInfo->Used=0;
			pInfo->Pos=0;
		}
		m_BufferLock.Unlock();
	}
}


bool CTSMemory::Finalize()
{
	// 終了処理

	if (m_fCloseAviUtl) {
		HWND hwndAviUtl=FindAviUtlWindow();

		if (::IsWindow(hwndAviUtl))
			::PostMessage(hwndAviUtl,WM_CLOSE,0,0);
	}

	FreeBuffer();

	m_BufferLock.Close();

	::DestroyWindow(m_hwnd);

	if (m_hMutex!=NULL)
		::CloseHandle(m_hMutex);

	return true;
}


struct EnumWindowsInfo {
	DWORD ProcessID;
	HWND hwnd;
};

static BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam)
{
	TCHAR szClassName[64];

	if (::GetClassName(hwnd,szClassName,64)>0
			&& ::lstrcmpi(szClassName,TEXT("AviUtl"))==0
			&& ::GetWindow(hwnd,GW_OWNER)==NULL) {
		EnumWindowsInfo *pInfo=(EnumWindowsInfo*)lParam;
		DWORD ProcessId=0;

		::GetWindowThreadProcessId(hwnd,&ProcessId);
		if (ProcessId==pInfo->ProcessID) {
			pInfo->hwnd=hwnd;
			return FALSE;
		}
	}
	return TRUE;
}


HWND CTSMemory::FindAviUtlWindow()
{
	if (m_AviUtlProcessID!=0) {
		// 既に起動しているプロセスのウィンドウを探す
		EnumWindowsInfo Info;
	
		Info.ProcessID=m_AviUtlProcessID;
		Info.hwnd=NULL;
		::EnumWindows(EnumWindowsProc,(LPARAM)&Info);
		if (::IsWindow(Info.hwnd))
			return Info.hwnd;
	}
	m_AviUtlProcessID=0;
	return NULL;
}


static HDROP CreateHDrop(LPCWSTR pszFileName)
{
	HDROP hDrop;
	LPDROPFILES pDropFile;

	hDrop=(HDROP)::GlobalAlloc(GHND,sizeof(DROPFILES)+(::lstrlenW(pszFileName)+2)*sizeof(WCHAR));
	if (hDrop==NULL)
		return NULL;
	pDropFile=(LPDROPFILES)::GlobalLock(hDrop);
	pDropFile->pFiles=sizeof(DROPFILES);
	pDropFile->pt.x=0;
	pDropFile->pt.y=0;
	pDropFile->fNC=FALSE;
	pDropFile->fWide=TRUE;
	::wsprintfW((LPWSTR)(pDropFile+1),L"%s%c",pszFileName,'\0');
	GlobalUnlock(hDrop);
	return hDrop;
}


static void ActivateWindow(HWND hwnd)
{
	HWND hwndFore=GetForegroundWindow();

	if (hwndFore!=hwnd) {
		DWORD ThreadID;

		ThreadID=GetWindowThreadProcessId(hwndFore,NULL);
		AttachThreadInput(GetCurrentThreadId(),ThreadID,TRUE);
		SetForegroundWindow(hwnd);
		AttachThreadInput(GetCurrentThreadId(),ThreadID,FALSE);
	}
}


// イベントコールバック関数
// 何かイベントが起きると呼ばれる
LRESULT CALLBACK CTSMemory::EventCallback(UINT Event,LPARAM lParam1,LPARAM lParam2,void *pClientData)
{
	CTSMemory *pThis=static_cast<CTSMemory*>(pClientData);

	switch (Event) {
	case TVTest::EVENT_PLUGINENABLE:
		// プラグインの有効状態が変化した
		if (lParam1!=0) {
			BOOL fOK=FALSE;

			if (!pThis->m_BufferLock.Lock())
				return FALSE;
			if (pThis->m_pBuffer==NULL) {
				const DWORD BufferSize=sizeof(BufferInfo)+pThis->m_BufferSize*188;
				SECURITY_DESCRIPTOR sd;
				SECURITY_ATTRIBUTES sa;

				::ZeroMemory(&sd,sizeof(sd));
				::InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
				::SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE);
				::ZeroMemory(&sa,sizeof(sa));
				sa.nLength=sizeof(sa);
				sa.lpSecurityDescriptor=&sd;
				pThis->m_BufferPos=0;
				pThis->m_BufferUsed=0;
				pThis->m_hFileMapping=::CreateFileMapping(INVALID_HANDLE_VALUE,
					&sa,PAGE_READWRITE,0,BufferSize,
					::PathFindFileName(pThis->m_szFileName));
				if (pThis->m_hFileMapping!=NULL) {
					if (::GetLastError()!=ERROR_ALREADY_EXISTS) {
						pThis->m_pBuffer=static_cast<BYTE*>(::MapViewOfFile(
							pThis->m_hFileMapping,FILE_MAP_WRITE,0,0,0));
						if (pThis->m_pBuffer!=NULL) {
							BufferInfo *pInfo=(BufferInfo*)pThis->m_pBuffer;
							pInfo->Size=pThis->m_BufferSize*188;
							pInfo->Used=0;
							pInfo->Pos=0;
							pInfo->Reserved=0;
							fOK=TRUE;
						}
					}
					if (!fOK) {
						::CloseHandle(pThis->m_hFileMapping);
						pThis->m_hFileMapping=NULL;
					}
				}
#ifdef _DEBUG
				else {
					TCHAR szText[256];
					::FormatMessage(
						FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,NULL,
						GetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),
						szText,256,NULL);
					MessageBox(pThis->m_pApp->GetAppWindow(),szText,NULL,MB_OK);
				}
#endif
			}
			pThis->m_BufferLock.Unlock();
			return fOK;
		} else {
			pThis->FreeBuffer();
		}
		return TRUE;

	case TVTest::EVENT_COMMAND:
		if (pThis->m_pBuffer!=NULL) {
			if (!::PathFileExists(pThis->m_szFileName)) {
				// ダミーファイル作成
				HANDLE hFile=::CreateFile(pThis->m_szFileName,GENERIC_WRITE,FILE_SHARE_READ,NULL,
					CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

				if (hFile==INVALID_HANDLE_VALUE) {
					::MessageBox(pThis->m_pApp->GetAppWindow(),
								 TEXT("ファイルが作成できません。"),NULL,
								 MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}
				::CloseHandle(hFile);
			}

			HWND hwndAviUtl=pThis->FindAviUtlWindow();
			if (::IsWindow(hwndAviUtl)) {
				HDROP hDrop;

				hDrop=CreateHDrop(pThis->m_szFileName);
				if (hDrop==NULL) {
					::MessageBox(NULL,TEXT("Can't create HDROP"),NULL,
								 MB_OK | MB_ICONEXCLAMATION);
					return TRUE;
				}
				if (pThis->m_fActivateAviUtl)
					ActivateWindow(hwndAviUtl);
				::PostMessage(hwndAviUtl,WM_DROPFILES,(WPARAM)hDrop,0);
				// hDropは勝手に開放されるみたい(使い回したらバグった...)
				return TRUE;
			}

			// AviUtlを起動する
			STARTUPINFO si;
			PROCESS_INFORMATION pi;
			TCHAR szFile[MAX_PATH*2+2];

			::ZeroMemory(&si,sizeof(si));
			si.cb=sizeof(si);
			si.dwFlags=STARTF_USESHOWWINDOW;
			if (pThis->m_fActivateAviUtl)
				si.wShowWindow=SW_SHOWNORMAL;
			else
				si.wShowWindow=SW_SHOWNOACTIVATE;
			// AviUtlは引数のパスを"で囲むとエラーになる
			::wsprintf(szFile,TEXT("\"%s\" %s"),
					   pThis->m_szAviUtlPath,pThis->m_szFileName);
			if (::CreateProcess(NULL,szFile,
								NULL,NULL,FALSE,NORMAL_PRIORITY_CLASS,
								NULL,NULL,&si,&pi)) {
				::CloseHandle(pi.hThread);
				::CloseHandle(pi.hProcess);
				pThis->m_AviUtlProcessID=pi.dwProcessId;
			} else {
				::MessageBox(pThis->m_pApp->GetAppWindow(),
							 !::PathFileExists(pThis->m_szAviUtlPath)?
							 TEXT("aviutl.exe のパスが間違っていると思われます。\n")
							 TEXT("TSMemory.ini の設定を見直してみてください。"):
							 TEXT("AviUtlを起動できません。"),NULL,
							 MB_OK | MB_ICONEXCLAMATION);
			}
		}
		return TRUE;

	case TVTest::EVENT_CHANNELCHANGE:
	case TVTest::EVENT_DRIVERCHANGE:
		pThis->PurgeBuffer();
		pThis->m_TsSelector.SetTargetServiceID(0,CTsSelector::STREAM_MPEG2VIDEO);
		return TRUE;

	case TVTest::EVENT_SERVICECHANGE:
	case TVTest::EVENT_SERVICEUPDATE:
		{
			int Service=pThis->m_pApp->GetService();

			if (Service>=0) {
				TVTest::ServiceInfo Info;

				if (pThis->m_pApp->GetServiceInfo(Service,&Info)) {
					pThis->m_TsSelector.SetTargetServiceID(Info.ServiceID,
														   CTsSelector::STREAM_MPEG2VIDEO);
				}
			}
		}
		return TRUE;
	}
	return 0;
}


// ストリームコールバック関数
// 188バイトのパケットデータが渡される
BOOL CALLBACK CTSMemory::StreamCallback(BYTE *pData,void *pClientData)
{
	CTSMemory *pThis=static_cast<CTSMemory*>(pClientData);

	pThis->m_TsPacket.SetData(pData,188);
	pThis->m_TsPacket.ParsePacket(pThis->m_ContCounter);
	pThis->m_TsSelector.InputMedia(&pThis->m_TsPacket);
	return TRUE;
}


CTSMemory *CTSMemory::GetThis(HWND hwnd)
{
	return reinterpret_cast<CTSMemory*>(::GetWindowLongPtr(hwnd,GWLP_USERDATA));
}


LRESULT CALLBACK CTSMemory::WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg) {
	case WM_CREATE:
		{
			LPCREATESTRUCT pcs=reinterpret_cast<LPCREATESTRUCT>(lParam);
			CTSMemory *pThis=static_cast<CTSMemory*>(pcs->lpCreateParams);

			::SetWindowLongPtr(hwnd,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd=hwnd;
		}
		return TRUE;

	case WM_DESTROY:
		{
			CTSMemory *pThis=GetThis(hwnd);

			pThis->m_hwnd=NULL;
		}
		return 0;
	}
	return ::DefWindowProc(hwnd,uMsg,wParam,lParam);
}


const bool CTSMemory::InputMedia(CMediaData *pMediaData, const DWORD dwInputIndex)
{
	if (static_cast<CTsPacket*>(pMediaData)->IsScrambled())
		return true;
	if (m_BufferLock.Lock()) {
		if (m_pBuffer!=NULL) {
			BufferInfo *pInfo=(BufferInfo*)m_pBuffer;

			::CopyMemory(m_pBuffer+sizeof(BufferInfo)+
								(m_BufferPos+m_BufferUsed)%m_BufferSize*188,
						 pMediaData->GetData(),188);
			if (m_BufferUsed<m_BufferSize) {
				m_BufferUsed++;
				pInfo->Used=m_BufferUsed*188;
			} else {
				m_BufferPos++;
				if (m_BufferPos==m_BufferSize)
					m_BufferPos=0;
				pInfo->Pos=m_BufferPos*188;
			}
		}
		m_BufferLock.Unlock();
	}
	return true;
}




TVTest::CTVTestPlugin *CreatePluginClass()
{
	return new CTSMemory;
}
