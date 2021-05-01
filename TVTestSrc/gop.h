/*******************************************************************
                            GOP interface
 *******************************************************************/

#ifndef GOP_H
#define GOP_H

#include "video_stream.h"
#include "timecode.h"
#include "sequence_header.h"
#include "picture_header.h"

#define BOTTOM_FIELD_FIRST 0
#define TOP_FIELD_FIRST    1

typedef struct {
	__int64  offset;
	__int64  start_frame;
	__int64  frame_count;

	SEQUENCE_HEADER *sh;
} GOP;

typedef struct {
	VIDEO_STREAM *p;

	/* fps */
	int rate;
	int scale;

	/* sequence start frame */
	__int64 start_frame;

	/* display/source field order */
	int field_order;
	
	READ_PICTURE_HEADER_OPTION *pic_opt;
	
} READ_GOP_PARAMETER;

typedef struct {
	void *arg1;
	GOP  (* func)(void *, __int64);
	void (* release)(void *);
} FIND_GOP;

#ifdef __cplusplus
extern "C" {
#endif

extern int next_gop(VIDEO_STREAM *p);
extern int last_gop(VIDEO_STREAM *p);
extern READ_GOP_PARAMETER *new_read_gop_parameter(VIDEO_STREAM *stream, SEQUENCE_HEADER *seq, READ_PICTURE_HEADER_OPTION *pic_opt, int field_order);
extern void delete_read_gop_parameter(void *p);
extern __int64 read_gop(READ_GOP_PARAMETER *in, GOP *out);
extern int skip_2nd_field(VIDEO_STREAM *in, READ_PICTURE_HEADER_OPTION *opt, PICTURE_HEADER *pic_1st);
extern __int64 count_frame(READ_GOP_PARAMETER *p);
extern GOP find_gop_with_timecode(void *p, __int64 frame);
	
#ifdef __cplusplus
}
#endif

#endif
