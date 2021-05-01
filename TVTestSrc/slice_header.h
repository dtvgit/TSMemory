/*******************************************************************
                        slice layer interface
 *******************************************************************/
#ifndef SLICE_HEADER_H
#define SLICE_HEADER_H

#include "video_stream.h"
#include "block.h"

typedef struct {
	int slice_vertical_position;
	int priority_break_point;
	int quantizer_scale_code;
	int intra_slice_flag;
	int intra_slice;
	int slice_picture_id_enable;
	int slice_picture_id;
} SLICE_HEADER;

typedef struct {
	int v_size;
	int scalable_mode;
} READ_SLICE_HEADER_OPTION;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SLICE_HEADER_C
extern int read_slice_header(VIDEO_STREAM *in, SLICE_HEADER *out, READ_SLICE_HEADER_OPTION *opt);
extern int slice_header_to_read_block_option(SLICE_HEADER *in, READ_BLOCK_OPTION *out);
#endif

#ifdef __cplusplus
}
#endif

#endif
