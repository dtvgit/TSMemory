/*
 *  64�r�b�g�����^�� DWORD �^��ɕ����Đ錾�����o�[�W����
 */
#ifndef VFAPI_H
#define VFAPI_H

#define	VF_STREAM_VIDEO 0x00000001
#define	VF_STREAM_AUDIO 0x00000002
#define	VF_OK           0x00000000
#define	VF_ERROR        0x80004005

typedef	struct {
	DWORD	dwSize;              /* ���̍\���̂̃T�C�Y */
	DWORD	dwAPIVersion;        /* Video File API �̃o�[�W���� */
	DWORD	dwVersion;           /* ���̃v���O�C���̃o�[�W���� */
	DWORD	dwSupportStreamType; /* ���̃v���O�C�����T�|�[�g����X�g���[���̎�� */
	                             /* �Ⴆ�Ήf���Ɖ������T�|�[�g���Ă���ꍇ�AVF_STREAM_VIDEO or VF_STREAM_AUDIO ������ */
        char	cPluginInfo[256];    /* ���̃v���O�C���̏��B��FAVI �t�@�C�����[�_�[ ver 1.0 */
	char	cFileType[256];      /* �t�@�C���_�C�A���O�Ŏg����t�B���^�B��FAVI �t�@�C��(*.avi)|*.avi */
} VF_PluginInfo,*LPVF_PluginInfo;

typedef	DWORD	VF_FileHandle,*LPVF_FileHandle;

typedef	struct {
	DWORD	dwSize;              /* ���̍\���̂̃T�C�Y */
	DWORD	dwHasStreams;        /* ���̃t�@�C�����ێ�����X�g���[���̎�ށB */
                                     /* �Ⴆ�Ήf���Ɖ���������ꍇ�AVF_STREAM_VIDEO or VF_STREAM_AUDIO ������ */
} VF_FileInfo,*LPVF_FileInfo;

typedef	struct {
	DWORD	dwSize;              /* ���̍\���̂̃T�C�Y */
	DWORD	dwLengthL;           /* �t���[����(�U�S�r�b�g����)�i���ʂR�Q�r�b�g�j */
	DWORD	dwLengthH;           /* �t���[����(�U�S�r�b�g����)�i��ʂR�Q�r�b�g�j*/
	DWORD	dwRate;              /* �t���[�����[�g�B���ۂ̃t���[�����[�g�� dwRate/dwScale �ŋ��߂� */
	DWORD	dwScale;             /* �t���[�����[�g�̃X�P�[�� */
	DWORD	dwWidth;             /* �f���̕� */
	DWORD	dwHeight;            /* �f���̍��� */
	DWORD	dwBitCount;          /* �f���̃r�b�g��(24 �̂�) */
} VF_StreamInfo_Video,*LPVF_StreamInfo_Video;

typedef	struct {
	DWORD	dwSize;              /* ���̍\���̂̃T�C�Y */
	DWORD	dwLengthL;           /* �T���v����(�U�S�r�b�g����)�i���ʂR�Q�r�b�g�j*/
	DWORD	dwLengthH;           /* �T���v����(�U�S�r�b�g����)�i��ʂR�Q�r�b�g�j*/
	DWORD	dwRate;              /* �T���v�����O���g���B���ۂ̃T���v�����O���g���� dwRate/dwScale �ŋ��߂� */
	DWORD	dwScale;             /* �T���v�����O���g���̃X�P�[�� */
	DWORD	dwChannels;          /* �`�����l����(1, 2 �̂����ꂩ) */
	DWORD	dwBitsPerSample;     /* �r�b�g��(8, 16 �̂����ꂩ) */
	DWORD	dwBlockAlign;        /* �P�T���v��������̃o�C�g�� */
} VF_StreamInfo_Audio,*LPVF_StreamInfo_Audio;

typedef	struct {
	DWORD	dwSize;              /* ���̍\���̂̃T�C�Y */
	DWORD	dwFrameNumberL;      /* �ǂݏo�������t���[���ԍ��i���ʂR�Q�r�b�g�j*/
	DWORD	dwFrameNumberH;      /* �ǂݏo�������t���[���ԍ��i��ʂR�Q�r�b�g�j*/
	void	*lpData;             /* �f���f�[�^�̊i�[��B�w�肵���x���W�̃��C���ւ̃|�C���^�� (lpData + lPitch*Y) �Ƃ��ċ��߂�B */
	long	lPitch;              /* ���̃��C���ւ̃I�t�Z�b�g */
} VF_ReadData_Video,*LPVF_ReadData_Video;

typedef	struct {
	DWORD	dwSize;              /* ���̍\���̂̃T�C�Y */
	DWORD	dwSamplePosL;        /* �ǂݏo�������T���v���ԍ��i���ʂR�Q�r�b�g�j*/
	DWORD	dwSamplePosH;        /* �ǂݏo�������T���v���ԍ��i��ʂR�Q�r�b�g�j*/
	DWORD	dwSampleCount;       /* �ǂݏo�������T���v���� */
	DWORD	dwReadedSampleCount; /* �ǂݏo�����T���v���� */
	DWORD	dwBufSize;           /* �o�b�t�@�̃T�C�Y */
	void	*lpBuf;              /* �ǂݏo����̃o�b�t�@ */
} VF_ReadData_Audio,*LPVF_ReadData_Audio;

typedef	struct {
	DWORD	dwSize;              /* ���̍\���̂̃T�C�Y */
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

