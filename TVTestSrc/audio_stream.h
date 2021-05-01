/*******************************************************************
                   MPEG Audio stream read interface
 *******************************************************************/

#ifndef AUDIO_STREAM_H
#define AUDIO_STREAM_H

typedef struct {
	__int64 sample;
	int     frequency;
	int     channel;
} AUDIO_INFO;

typedef struct {
	int           stream;
	
	void         (* close)(void *audio_stream);
	__int64      (* tell)(int stream);
	__int64      (* seek)(int stream, __int64 sample);
	int          (* read)(int stream, void *buffer, int size);
	unsigned int (* next_sync)(int stream);
	void         (* get_info)(int stream, AUDIO_INFO *info);
} AUDIO_STREAM;

#ifdef __cplusplus
extern "C" {
#endif

extern AUDIO_STREAM *audio_stream_open(char *path);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_STREAM_H */
