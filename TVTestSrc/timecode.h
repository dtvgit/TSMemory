/*******************************************************************
                       MPEG time code format
 *******************************************************************/
#ifndef TIMECODE_H
#define TIMECODE_H

#include "video_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int drop;    /* 1 bit  */
	int hh;      /* 5 bits */
	int mm;      /* 6 bits */
	int padding; /* 1 bit  */
	int ss;      /* 6 bits */
	int ff;      /* 6 bits */
} TIMECODE;

#ifndef TIMECODE_C
extern void read_timecode(VIDEO_STREAM *in, TIMECODE *out);
extern __int64 timecode2frame(TIMECODE *in, int fps);
#endif

#ifdef __cplusplus
}
#endif

#endif
