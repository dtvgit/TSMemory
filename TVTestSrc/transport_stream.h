/*******************************************************************
                     Transport Stream interface
 *******************************************************************/
#ifndef TRANSPORT_STREAM_H
#define TRANSPORT_STREAM_H

#include "stream_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRANSPROT_STREAM_C
extern int ts_open(const char *filename, int stream_type);
extern int ts_close(int in);
extern int ts_read(int in, void *data, unsigned int count);
extern __int64 ts_seek(int in, __int64 offset, int origin);
extern __int64 ts_tell(int in);
#endif

#ifdef __cplusplus
}
#endif

#endif