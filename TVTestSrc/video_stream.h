/*******************************************************************
                MPEG VIDEO stream read interface
 *******************************************************************/

#ifndef VIDEO_STREAM_H
#define VIDEO_STREAM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIDEO_STREAM_BUFFER_SIZE   512*1024
#define VIDEO_STREAM_BUFFER_MARGIN 8
	
typedef struct {
	__int64        file_length;

	__int64        last_position; 

	unsigned int   bits;

	int            fd;
	int            buffer_size;
	int            bits_rest;

	unsigned char *current;
	unsigned char  buffer[VIDEO_STREAM_BUFFER_MARGIN+VIDEO_STREAM_BUFFER_SIZE];
	char           path[FILENAME_MAX];

	int           (* close)(int);
	int           (* read)(int, void *, unsigned int);
	__int64       (* seek)(int, __int64, int);
	__int64       (* tell)(int);
	
} VIDEO_STREAM;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VIDEO_STREAM_C
extern int open_video_stream(char *path, VIDEO_STREAM *out);
extern int close_video_stream(VIDEO_STREAM *p);
extern int vs_get_bits(VIDEO_STREAM *in, int num_of_bits);
extern int vs_read_bits(VIDEO_STREAM *in, int num_of_bits);
extern int vs_erase_bits(VIDEO_STREAM *in, int num_of_bits);
extern int vs_next_start_code(VIDEO_STREAM *in);
extern __int64 video_stream_tell(VIDEO_STREAM *p);
extern __int64 video_stream_seek(VIDEO_STREAM *p, __int64 offset, int origin);
#endif

#ifdef __cplusplus
}
#endif

#endif
