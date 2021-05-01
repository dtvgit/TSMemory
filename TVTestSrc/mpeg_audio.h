/*******************************************************************
                   MPEG Audio stream decode interface
 *******************************************************************/
#ifndef MPEG_AUDIO_H
#define MEPG_AUDIO_H

#include <windows.h>

#include "audio_stream.h"
#include "pcm_buffer.h"
#include "layer2.h"

typedef struct {
	AUDIO_STREAM    *as;
	PCM_BUFFER       pcm;
	LAYER2_WORK      work;
	unsigned char    unit[2048];
	
	__int64          sample;
	int              frequency;
	int              channel;

	CRITICAL_SECTION lock;
	
} MPEG_AUDIO;

#ifndef MPEG_AUDIO_C
#ifdef __cplusplus
extern "C" {
#endif

extern MPEG_AUDIO *open_mpeg_audio(char *path);
extern void close_mpeg_audio(MPEG_AUDIO *p);
extern int read_mpeg_audio(MPEG_AUDIO *p, __int64 offset, void *buffer, int length);

#ifdef __cplusplus
}
#endif
#endif /* MPEG_AUDIO_C */

#endif /* MPEG_AUDIO_H */
