/*******************************************************************
                MC - Motion Compensation interface
 *******************************************************************/
#ifndef MC_H
#define MC_H

#include "frame.h"

#define PREDICTION_TYPE_FIELD_BASED 1
#define PREDICTION_TYPE_FRAME_BASED 2
#define PREDICTION_TYPE_DUAL_PRIME  3
#define PREDICTION_TYPE_16x8_MC     4

#define MOTION_VECTOR_FORMAT_FIELD 1
#define MOTION_VECTOR_FORMAT_FRAME 2

typedef struct {
	FRAME *forward;
	FRAME *backward;
	FRAME *current;
}MC_BUFFER;

typedef void (* PREDICTION_FUNC)(FRAME *, FRAME *, int, int, int, int, int, int, int);

typedef struct {
	int chroma_format;
	
	int first_field;
	
	int picture_structure;
	int picture_coding_type;
	int top_field_first;
	
	int macroblock_motion_forward;
	int macroblock_motion_backward;
	
	int prediction_type;
	int motion_vertical_field_select[2][2];
	int PMV[2][2][2];
	int DMV[2][2];

	PREDICTION_FUNC prediction_func;
} MC_PARAMETER;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MC_C
extern int mc(MC_BUFFER *buf, MC_PARAMETER *opt, int x, int y);
extern void prediction(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format);
extern void prediction_mmx(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format);
extern void prediction_sse(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format);
extern void prediction_sse2(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format);
#endif

#ifdef __cplusplus
}
#endif

#endif
