/*******************************************************************
           picture decoding (reconstracting) interface
 *******************************************************************/
#ifndef PICTURE_H
#define PICTURE_H

#include "block.h"
#include "mc.h"
#include "macroblock.h"
#include "slice_header.h"
#include "idct.h"
#include "frame.h"

typedef void (* ADD_BLOCK)(short *, FRAME *, READ_BLOCK_OPTION *, int, int, int);

typedef struct {
	READ_BLOCK_OPTION         block_option;
	MC_PARAMETER              mc_parameter;
	READ_MACROBLOCK_OPTION    macroblock_option;
	READ_SLICE_HEADER_OPTION  slice_option;
	IDCT                      idct_func;
	ADD_BLOCK                 add_block_func;
} DECODE_PICTURE_PARAMETER;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PICTURE_C
extern int decode_picture(VIDEO_STREAM *in, MC_BUFFER *out, DECODE_PICTURE_PARAMETER *prm);
extern void add_block_data_to_frame(short *in, FRAME *out, READ_BLOCK_OPTION *opt, int x, int y, int block_number);
extern void add_block_data_to_frame_mmx(short *in, FRAME *out, READ_BLOCK_OPTION *opt, int x, int y, int block_number);
#endif

#ifdef __cplusplus
}
#endif
	
#endif
