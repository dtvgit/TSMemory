/*******************************************************************
                         GOP List interfaces
 *******************************************************************/

#ifndef GOP_LIST_H
#define GOP_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include "video_stream.h"
#include "sequence_header.h"
#include "picture_header.h"
#include "gop.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	__int64          stream_length;
	__int64          num_of_frame;
	int              num_of_gop;
	int              num_of_sh;
	GOP             *gop;
	SEQUENCE_HEADER *sh;
} GOP_LIST;

extern GOP_LIST *new_gop_list(VIDEO_STREAM *in, READ_PICTURE_HEADER_OPTION *opt, int field_order, int dialog_mode);
extern void delete_gop_list(void *gop_list);
extern GOP find_gop_with_gop_list(void *p, __int64 frame);
extern int store_gop_list(GOP_LIST *in, char *filepath);
extern GOP_LIST *load_gop_list(char *filepath);

#ifdef __cplusplus
}
#endif

#endif
