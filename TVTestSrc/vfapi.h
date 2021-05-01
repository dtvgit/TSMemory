/*
 *  64ビット整数型を DWORD 型二つに分けて宣言したバージョン
 */
#ifndef VFAPI_H
#define VFAPI_H

#define	VF_STREAM_VIDEO 0x00000001
#define	VF_STREAM_AUDIO 0x00000002
#define	VF_OK           0x00000000
#define	VF_ERROR        0x80004005

typedef	struct {
	DWORD	dwSize;              /* この構造体のサイズ */
	DWORD	dwAPIVersion;        /* Video File API のバージョン */
	DWORD	dwVersion;           /* このプラグインのバージョン */
	DWORD	dwSupportStreamType; /* このプラグインがサポートするストリームの種類 */
	                             /* 例えば映像と音声をサポートしている場合、VF_STREAM_VIDEO or VF_STREAM_AUDIO が入る */
        char	cPluginInfo[256];    /* このプラグインの情報。例：AVI ファイルリーダー ver 1.0 */
	char	cFileType[256];      /* ファイルダイアログで使われるフィルタ。例：AVI ファイル(*.avi)|*.avi */
} VF_PluginInfo,*LPVF_PluginInfo;

typedef	DWORD	VF_FileHandle,*LPVF_FileHandle;

typedef	struct {
	DWORD	dwSize;              /* この構造体のサイズ */
	DWORD	dwHasStreams;        /* このファイルが保持するストリームの種類。 */
                                     /* 例えば映像と音声がある場合、VF_STREAM_VIDEO or VF_STREAM_AUDIO が入る */
} VF_FileInfo,*LPVF_FileInfo;

typedef	struct {
	DWORD	dwSize;              /* この構造体のサイズ */
	DWORD	dwLengthL;           /* フレーム数(６４ビット整数)（下位３２ビット） */
	DWORD	dwLengthH;           /* フレーム数(６４ビット整数)（上位３２ビット）*/
	DWORD	dwRate;              /* フレームレート。実際のフレームレートは dwRate/dwScale で求める */
	DWORD	dwScale;             /* フレームレートのスケール */
	DWORD	dwWidth;             /* 映像の幅 */
	DWORD	dwHeight;            /* 映像の高さ */
	DWORD	dwBitCount;          /* 映像のビット数(24 のみ) */
} VF_StreamInfo_Video,*LPVF_StreamInfo_Video;

typedef	struct {
	DWORD	dwSize;              /* この構造体のサイズ */
	DWORD	dwLengthL;           /* サンプル数(６４ビット整数)（下位３２ビット）*/
	DWORD	dwLengthH;           /* サンプル数(６４ビット整数)（上位３２ビット）*/
	DWORD	dwRate;              /* サンプリング周波数。実際のサンプリング周波数は dwRate/dwScale で求める */
	DWORD	dwScale;             /* サンプリング周波数のスケール */
	DWORD	dwChannels;          /* チャンネル数(1, 2 のいずれか) */
	DWORD	dwBitsPerSample;     /* ビット数(8, 16 のいずれか) */
	DWORD	dwBlockAlign;        /* １サンプル当たりのバイト数 */
} VF_StreamInfo_Audio,*LPVF_StreamInfo_Audio;

typedef	struct {
	DWORD	dwSize;              /* この構造体のサイズ */
	DWORD	dwFrameNumberL;      /* 読み出したいフレーム番号（下位３２ビット）*/
	DWORD	dwFrameNumberH;      /* 読み出したいフレーム番号（上位３２ビット）*/
	void	*lpData;             /* 映像データの格納先。指定したＹ座標のラインへのポインタは (lpData + lPitch*Y) として求める。 */
	long	lPitch;              /* 次のラインへのオフセット */
} VF_ReadData_Video,*LPVF_ReadData_Video;

typedef	struct {
	DWORD	dwSize;              /* この構造体のサイズ */
	DWORD	dwSamplePosL;        /* 読み出したいサンプル番号（下位３２ビット）*/
	DWORD	dwSamplePosH;        /* 読み出したいサンプル番号（上位３２ビット）*/
	DWORD	dwSampleCount;       /* 読み出したいサンプル数 */
	DWORD	dwReadedSampleCount; /* 読み出したサンプル数 */
	DWORD	dwBufSize;           /* バッファのサイズ */
	void	*lpBuf;              /* 読み出し先のバッファ */
} VF_ReadData_Audio,*LPVF_ReadData_Audio;

typedef	struct {
	DWORD	dwSize;              /* この構造体のサイズ */
	HRESULT (_stdcall *OpenFile)( char *lpFileName,LPVF_FileHandle lpFileHandle);
	HRESULT (_stdcall *CloseFile)( VF_FileHandle hFileHandle);
	HRESULT (_stdcall *GetFileInfo)( VF_FileHandle hFileHandle,LPVF_FileInfo lpFileInfo );
	HRESULT (_stdcall *GetStreamInfo)( VF_FileHandle hFileHandle,DWORD dwStream,void *lpStreamInfo );
	HRESULT (_stdcall *ReadData)( VF_FileHandle hFileHandle,DWORD dwStream,void *lpData ); 
} VF_PluginFunc,*LPVF_PluginFunc;

/*
HRESULT _stdcall vfGetPluginInfo( LPVF_PluginInfo lpPluginInfo );
HRESULT _stdcall vfGetPluginFunc( LPVF_PluginFunc lpPluginFunc );
*/

#endif /* VFAPI_H */

