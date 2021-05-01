/*******************************************************************
                    Program Stream interfaces
 *******************************************************************/
#ifndef PROGRAM_STREAM_H
#define PROGRAM_STREAM_H

#include "stream_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PROGRAM_STREAM_C
extern int ps_open(const char *filename, int stream_type);
extern int ps_close(int in);
extern int ps_read(int in, void *data, unsigned int count);
extern __int64 ps_seek(int in, __int64 offset, int origin);
extern __int64 ps_tell(int in);
#endif
	
#ifdef __cplusplus
}
#endif

#endif	