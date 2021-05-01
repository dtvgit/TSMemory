/*******************************************************************
                   MPEG-2 stream edit interface
 *******************************************************************/
#ifndef MPEG2EDIT_H
#define MPEG2EDIT_H

#include <windows.h>

typedef struct {
	void  *stream;
	void (*edit)(void *mpeg2edit, char *out_path, __int64 in, __int64 out);
	void (*close)(void *mpeg2edit);
	HRESULT (_stdcall *callback)(char *out_path, DWORD percent);
} MPEG2EDIT;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MPEG2EDIT_C
extern int open_mpeg2edit(const char *path, MPEG2EDIT *mpeg2_edit);
#endif

#ifdef __cplusplus
}
#endif

#endif /* MPEG2EDIT_H */

