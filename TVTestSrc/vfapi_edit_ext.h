/*******************************************************************
 VFAPI Edit Extension

   2001,12/6 draft version by MOGI, Kazuhiro

   LOG:
     2001, 10/28 create this file
           12/6  add VF_CutOption structure
                 add Cut() function on VF_PluginFuncEditExt structure

 *******************************************************************/

#ifndef VFAPI_EDIT_EXT_H
#define VFAPI_EDIT_EXT_H

#include "vfapi.h"

#define VF_FRAME_TYPE_UNEDITABLE 0x00000000
#define VF_FRAME_TYPE_STARTABLE  0x00000001
#define VF_FRAME_TYPE_ENDABLE    0x00000002

typedef struct {
	DWORD    dwSize;            /* sizeof(VF_FrameType) */
	DWORD    dwFrameNumberL;    /* request frame number (lower 32 bit) */
	DWORD    dwFrameNumberH;    /* request frame number (higher 32 bit) */
	DWORD    dwFrameType;       /* result */
} VF_FrameType, *LPVF_FrameType;

typedef struct {
	DWORD    dwSize;            /* sizeof(VF_CutOption)      */
	char    *lpOutputFileName;
	DWORD    dwInPointL;        /* in-point (lower 32 bit)   */
	DWORD    dwInPointH;        /* in-point (higher 32 bit)  */
	DWORD    dwOutPointL;       /* out-point (lower 32 bit)  */
	DWORD    dwOutPointH;       /* out-point (higher 32 bit) */
	HRESULT (_stdcall *Callback)( char *lpOutputFileName, DWORD dwPercent );
} VF_CutOption, *LPVF_CutOption;

typedef struct {
	DWORD    dwSize; /* sizeof(VF_PluginFuncEditExt) */
	HRESULT (_stdcall *GetFrameType)( VF_FileHandle hFileHandle, LPVF_FrameType lpFrameType );
	HRESULT (_stdcall *Cut)( VF_FileHandle hFileHandle, LPVF_CutOption lpCutOption );
} VF_PluginFuncEditExt, *LPVF_PluginFuncEditExt;

/*
HRESULT _stdcall vfGetPluginFuncEditExt( LPVF_PluginFuncEditExt lpPluginFuncEditExt);
*/

#endif /* VFAPI_EDIT_EXT_H */
