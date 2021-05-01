/*******************************************************************
                MC - Motion Compensation module
 *******************************************************************/
#include <string.h>

#define MC_C
#include "mc.h"

#define FROM_FRAME 0x00
#define FROM_TOP   0x10
#define FROM_BOTOM 0x20
#define TO_FRAME   0x00
#define TO_TOP     0x01
#define TO_BOTOM   0x02
#define PREDICTION_FRAME_TO_FRAME (FROM_FRAME|TO_FRAME)
#define PREDICTION_FRAME_TO_TOP   (FROM_FRAME|TO_TOP)
#define PREDICTION_FRAME_TO_BOTOM (FROM_FRAME|TO_BOTOM)
#define PREDICTION_TOP_TO_TOP     (FROM_TOP|TO_TOP)
#define PREDICTION_TOP_TO_BOTOM   (FROM_TOP|TO_BOTOM)
#define PREDICTION_BOTOM_TO_TOP   (FROM_BOTOM|TO_TOP)
#define PREDICTION_BOTOM_TO_BOTOM (FROM_BOTOM|TO_BOTOM)

int mc(MC_BUFFER *buf, MC_PARAMETER *prm, int x, int y);

void prediction(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format);

static void prediction_w16_hh_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w16_hh_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w16_fh_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w16_fh_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w16_hf_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w16_hf_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w16_ff_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w16_ff_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w8_hh_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w8_hh_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w8_fh_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w8_fh_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w8_hf_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w8_hf_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w8_ff_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
static void prediction_w8_ff_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);


void prediction_mmx(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format);

extern void __stdcall prediction_w16_hh_1st_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_hh_2nd_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_fh_1st_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_fh_2nd_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_hf_1st_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_hf_2nd_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_ff_1st_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_ff_2nd_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_hh_1st_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_hh_2nd_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_fh_1st_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_fh_2nd_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_hf_1st_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_hf_2nd_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_ff_1st_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_ff_2nd_mmx(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);

void prediction_sse(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format);

extern void __stdcall prediction_w16_hh_2nd_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_fh_1st_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_fh_2nd_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_hf_1st_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_hf_2nd_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_ff_2nd_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_fh_1st_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_fh_2nd_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_hf_1st_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_hf_2nd_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w8_ff_2nd_sse(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);

void prediction_sse2(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format);

extern void __stdcall prediction_w16_hh_1st_sse2(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_hh_2nd_sse2(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_fh_1st_sse2(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_fh_2nd_sse2(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_hf_1st_sse2(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_hf_2nd_sse2(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);
extern void __stdcall prediction_w16_ff_2nd_sse2(unsigned char *in, unsigned char *out, int in_step, int out_step, int height);

int mc(MC_BUFFER *buf, MC_PARAMETER *prm, int x, int y)
{
	int first;
	int current_field;
	int type;
	FRAME *prediction_frame;

	first = 1;
	
	if( prm->macroblock_motion_forward || (prm->picture_coding_type == 2) ){
		if(prm->picture_structure == 3){
			if( (prm->prediction_type == PREDICTION_TYPE_FRAME_BASED) || !prm->macroblock_motion_forward ){
				
				prm->prediction_func(buf->forward, buf->current, PREDICTION_FRAME_TO_FRAME, prm->PMV[0][0][0], prm->PMV[0][0][1], x, y, first, prm->chroma_format);
				
			}else if(prm->prediction_type == PREDICTION_TYPE_FIELD_BASED){
				
				if(prm->motion_vertical_field_select[0][0]){
					prm->prediction_func(buf->forward, buf->current, PREDICTION_BOTOM_TO_TOP, prm->PMV[0][0][0], prm->PMV[0][0][1] >> 1, x, y, first, prm->chroma_format);
				}else{
					prm->prediction_func(buf->forward, buf->current, PREDICTION_TOP_TO_TOP, prm->PMV[0][0][0], prm->PMV[0][0][1] >> 1, x, y, first, prm->chroma_format);
				}

				if(prm->motion_vertical_field_select[1][0]){
					prm->prediction_func(buf->forward, buf->current, PREDICTION_BOTOM_TO_BOTOM, prm->PMV[1][0][0], prm->PMV[1][0][1] >> 1, x, y, first, prm->chroma_format);
				}else{
					prm->prediction_func(buf->forward, buf->current, PREDICTION_TOP_TO_BOTOM, prm->PMV[1][0][0], prm->PMV[1][0][1] >> 1, x, y, first, prm->chroma_format);
				}
				
			}else if(prm->prediction_type == PREDICTION_TYPE_DUAL_PRIME){

				prm->prediction_func(buf->forward, buf->current, PREDICTION_TOP_TO_TOP, prm->PMV[0][0][0], prm->PMV[0][0][1]>>1, x, y, 1, prm->chroma_format);
				prm->prediction_func(buf->forward, buf->current, PREDICTION_BOTOM_TO_TOP, prm->DMV[0][0], prm->DMV[0][1], x, y, 0, prm->chroma_format);
				
				prm->prediction_func(buf->forward, buf->current, PREDICTION_BOTOM_TO_BOTOM, prm->PMV[1][0][0], prm->PMV[1][0][1]>>1, x, y, 1, prm->chroma_format);
				prm->prediction_func(buf->forward, buf->current, PREDICTION_TOP_TO_BOTOM, prm->DMV[1][0], prm->DMV[1][1], x, y, 0, prm->chroma_format);

			}
		}else{
			
			if(prm->picture_structure == 2){
				type = TO_BOTOM;
				current_field = 1;
			}else{
				type = TO_TOP;
				current_field = 0;
			}

			if(prm->motion_vertical_field_select[0][0]){
				type += FROM_BOTOM;
			}else{
				type += FROM_TOP;
			}

			if( (prm->picture_coding_type == 2) && !(prm->first_field) && (current_field != prm->motion_vertical_field_select[0][0]) ){
				prediction_frame = buf->current;
			}else{
				prediction_frame = buf->forward;
			}

			if( (prm->prediction_type == PREDICTION_TYPE_FIELD_BASED) || !prm->macroblock_motion_forward ){
				
				prm->prediction_func(prediction_frame, buf->current, type, prm->PMV[0][0][0], prm->PMV[0][0][1], x, y, first, prm->chroma_format);
				prm->prediction_func(prediction_frame, buf->current, type, prm->PMV[0][0][0], prm->PMV[0][0][1], x, y+16, first, prm->chroma_format);
				
			}else if(prm->prediction_type == PREDICTION_TYPE_16x8_MC){

				prm->prediction_func(prediction_frame, buf->current, type, prm->PMV[0][0][0], prm->PMV[0][0][1], x, y, first, prm->chroma_format);

				if(prm->motion_vertical_field_select[1][0]){
					type = (type & 0xf) + FROM_BOTOM;
				}else{
					type = (type & 0xf) + FROM_TOP;
				}

				if( (prm->picture_coding_type == 2) && !(prm->first_field) && (current_field != prm->motion_vertical_field_select[1][0]) ){
					prediction_frame = buf->current;
				}else{
					prediction_frame = buf->forward;
				}
				
				prm->prediction_func(prediction_frame, buf->current, type, prm->PMV[1][0][0], prm->PMV[1][0][1], x, y+16, first, prm->chroma_format);

			}else if(prm->prediction_type == PREDICTION_TYPE_DUAL_PRIME){
				
				if(prm->first_field){
					prediction_frame = buf->forward;
				}else{
					prediction_frame = buf->current;
				}

				type &= 0xF;
				type |= type << 4;
				
				prm->prediction_func(buf->forward, buf->current, type, prm->PMV[0][0][0], prm->PMV[0][0][1], x, y, 1, prm->chroma_format);
				prm->prediction_func(buf->forward, buf->current, type, prm->PMV[0][0][0], prm->PMV[0][0][1], x, y+16, 1, prm->chroma_format);

				type &= 0xF;
				if(type == TO_TOP){
					type |= FROM_BOTOM;
				}else{
					type |= FROM_TOP;
				}
				
				prm->prediction_func(prediction_frame, buf->current, type, prm->DMV[0][0], prm->DMV[0][1], x, y, 0, prm->chroma_format);
				prm->prediction_func(prediction_frame, buf->current, type, prm->DMV[0][0], prm->DMV[0][1], x, y+16, 0, prm->chroma_format);

			}
		}
		first = 0;
	}

	if(prm->macroblock_motion_backward){
		if(prm->picture_structure == 3){
			if(prm->prediction_type == PREDICTION_TYPE_FRAME_BASED){
				
				prm->prediction_func(buf->backward, buf->current, PREDICTION_FRAME_TO_FRAME, prm->PMV[0][1][0], prm->PMV[0][1][1], x, y, first, prm->chroma_format);
				
			}else{
				
				if(prm->motion_vertical_field_select[0][1]){
					type = PREDICTION_BOTOM_TO_TOP;
				}else{
					type = PREDICTION_TOP_TO_TOP;
				}
				prm->prediction_func(buf->backward, buf->current, type, prm->PMV[0][1][0], prm->PMV[0][1][1] >> 1, x, y, first, prm->chroma_format);

				if(prm->motion_vertical_field_select[1][1]){
					type = PREDICTION_BOTOM_TO_BOTOM;
				}else{
					type = PREDICTION_TOP_TO_BOTOM;
				}
				prm->prediction_func(buf->backward, buf->current, type, prm->PMV[1][1][0], prm->PMV[1][1][1] >> 1, x, y, first, prm->chroma_format);
				
			}
		}else{
			
			if(prm->picture_structure == 2){
				type = TO_BOTOM;
			}else{
				type = TO_TOP;
			}

			if(prm->motion_vertical_field_select[0][1]){
				type += FROM_BOTOM;
			}else{
				type += FROM_TOP;
			}

			if(prm->prediction_type == PREDICTION_TYPE_FIELD_BASED){

				prm->prediction_func(buf->backward, buf->current, type, prm->PMV[0][1][0], prm->PMV[0][1][1], x, y, first, prm->chroma_format);
				prm->prediction_func(buf->backward, buf->current, type, prm->PMV[0][1][0], prm->PMV[0][1][1], x, y+16, first, prm->chroma_format);
				
			}else if(prm->prediction_type == PREDICTION_TYPE_16x8_MC){

				prm->prediction_func(buf->backward, buf->current, type, prm->PMV[0][1][0], prm->PMV[0][1][1], x, y, first, prm->chroma_format);
				
				if(prm->motion_vertical_field_select[1][1]){
					type = (type & 0xf) + FROM_BOTOM;
				}else{
					type = (type & 0xf) + FROM_TOP;
				}

				prm->prediction_func(buf->backward, buf->current, type, prm->PMV[1][1][0], prm->PMV[1][1][1], x, y+16, first, prm->chroma_format);
				
			}
		}
	}

	return 1;
}
				
typedef void (__stdcall *ASM_MC_CORE)(unsigned char *, unsigned char *, int, int, int);

void prediction_mmx(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format)
{
	int r_offset, p_offset;
	int r_step, p_step;
	int width, height;
	int xh, yh;
	int xiv, yiv;
	unsigned char *r, *p;

	static const ASM_MC_CORE table[2][2][2][2] = {
		{/* width - 8 */
			{ /* horizontal full */
				{ /* vertical full */
					prediction_w8_ff_2nd_mmx,
					prediction_w8_ff_1st_mmx,
				},
				{ /* vertical half */
					prediction_w8_fh_2nd_mmx,
					prediction_w8_fh_1st_mmx,
				},
			},
			{ /* horizontal half */
				{ /* vertical full */
					prediction_w8_hf_2nd_mmx,
					prediction_w8_hf_1st_mmx,
				},
				{ /* vertical half */
					prediction_w8_hh_2nd_mmx,
					prediction_w8_hh_1st_mmx,
				},
			},
		},
		{/* width - 16 */
			{ /* horizontal full */
				{ /* vertical full */
					prediction_w16_ff_2nd_mmx,
					prediction_w16_ff_1st_mmx,
				},
				{ /* vertical half */
					prediction_w16_fh_2nd_mmx,
					prediction_w16_fh_1st_mmx,
				},
			},
			{ /* horizontal half */
				{ /* vertical full */
					prediction_w16_hf_2nd_mmx,
					prediction_w16_hf_1st_mmx,
				},
				{ /* vertical half */
					prediction_w16_hh_2nd_mmx,
					prediction_w16_hh_1st_mmx,
				},
			},
		},
	};

	switch(type){
	case PREDICTION_FRAME_TO_FRAME:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width;
		p_step = in->width;
		width = 16;
		height = 16;
		break;
	case PREDICTION_FRAME_TO_TOP:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_FRAME_TO_BOTOM:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_TOP_TO_TOP:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_TOP_TO_BOTOM:
		r_offset = in->width * y + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_BOTOM_TO_TOP:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * y + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_BOTOM_TO_BOTOM:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	default:
		return;
	}

	r = in->y + r_offset;
	p = out->y + p_offset;

	xh = xv & 1;
	yh = yv & 1;
	xiv = xv >> 1;
	yiv = yv >> 1;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);

	if(chroma_format != 3){
		width >>= 1;
		xv /= 2;
		x /= 2;
		r_offset -= x;
		p_offset -= x;
	}
	if(chroma_format == 1){
		height >>= 1;
		yv /= 2;
		y /= 2;
		r_offset -= in->width * y;
		p_offset -= in->width * y;
	}

	r = in->u + r_offset;
	p = out->u + p_offset;

	xh = xv & 1;
	yh = yv & 1;
	xiv = xv >> 1;
	yiv = yv >> 1;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);

	r = in->v + r_offset;
	p = out->v + p_offset;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);
}

void prediction_sse(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format)
{
	int r_offset, p_offset;
	int r_step, p_step;
	int width, height;
	int xh, yh;
	int xiv, yiv;
	unsigned char *r, *p;

	static const ASM_MC_CORE table[2][2][2][2] = {
		{/* width - 8 */
			{ /* horizontal full */
				{ /* vertical full */
					prediction_w8_ff_2nd_sse,
					prediction_w8_ff_1st_mmx,
				},
				{ /* vertical half */
					prediction_w8_fh_2nd_sse,
					prediction_w8_fh_1st_sse,
				},
			},
			{ /* horizontal half */
				{ /* vertical full */
					prediction_w8_hf_2nd_sse,
					prediction_w8_hf_1st_sse,
				},
				{ /* vertical half */
					prediction_w8_hh_2nd_mmx,
					prediction_w8_hh_1st_mmx,
				},
			},
		},
		{/* width - 16 */
			{ /* horizontal full */
				{ /* vertical full */
					prediction_w16_ff_2nd_sse,
					prediction_w16_ff_1st_mmx,
				},
				{ /* vertical half */
					prediction_w16_fh_2nd_sse,
					prediction_w16_fh_1st_sse,
				},
			},
			{ /* horizontal half */
				{ /* vertical full */
					prediction_w16_hf_2nd_sse,
					prediction_w16_hf_1st_sse,
				},
				{ /* vertical half */
					prediction_w16_hh_2nd_mmx,
					prediction_w16_hh_1st_mmx,
				},
			},
		},
	};

	switch(type){
	case PREDICTION_FRAME_TO_FRAME:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width;
		p_step = in->width;
		width = 16;
		height = 16;
		break;
	case PREDICTION_FRAME_TO_TOP:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_FRAME_TO_BOTOM:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_TOP_TO_TOP:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_TOP_TO_BOTOM:
		r_offset = in->width * y + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_BOTOM_TO_TOP:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * y + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_BOTOM_TO_BOTOM:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	default:
		return;
	}

	r = in->y + r_offset;
	p = out->y + p_offset;

	xh = xv & 1;
	yh = yv & 1;
	xiv = xv >> 1;
	yiv = yv >> 1;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);

	if(chroma_format != 3){
		width >>= 1;
		xv /= 2;
		x /= 2;
		r_offset -= x;
		p_offset -= x;
	}
	if(chroma_format == 1){
		height >>= 1;
		yv /= 2;
		y /= 2;
		r_offset -= in->width * y;
		p_offset -= in->width * y;
	}

	r = in->u + r_offset;
	p = out->u + p_offset;

	xh = xv & 1;
	yh = yv & 1;
	xiv = xv >> 1;
	yiv = yv >> 1;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);

	r = in->v + r_offset;
	p = out->v + p_offset;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);
}

void prediction_sse2(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format)
{
	int r_offset, p_offset;
	int r_step, p_step;
	int width, height;
	int xh, yh;
	int xiv, yiv;
	unsigned char *r, *p;

	static const ASM_MC_CORE table[2][2][2][2] = {
		{/* width - 8 */
			{ /* horizontal full */
				{ /* vertical full */
					prediction_w8_ff_2nd_sse,
					prediction_w8_ff_1st_mmx,
				},
				{ /* vertical half */
					prediction_w8_fh_2nd_sse,
					prediction_w8_fh_1st_sse,
				},
			},
			{ /* horizontal half */
				{ /* vertical full */
					prediction_w8_hf_2nd_sse,
					prediction_w8_hf_1st_sse,
				},
				{ /* vertical half */
					prediction_w8_hh_2nd_mmx,
					prediction_w8_hh_1st_mmx,
				},
			},
		},
		{/* width - 16 */
			{ /* horizontal full */
				{ /* vertical full */
					prediction_w16_ff_2nd_sse2,
					prediction_w16_ff_1st_mmx,
				},
				{ /* vertical half */
					prediction_w16_fh_2nd_sse2,
					prediction_w16_fh_1st_sse2,
				},
			},
			{ /* horizontal half */
				{ /* vertical full */
					prediction_w16_hf_2nd_sse2,
					prediction_w16_hf_1st_sse2,
				},
				{ /* vertical half */
					prediction_w16_hh_2nd_sse2,
					prediction_w16_hh_1st_sse2,
				},
			},
		},
	};

	switch(type){
	case PREDICTION_FRAME_TO_FRAME:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width;
		p_step = in->width;
		width = 16;
		height = 16;
		break;
	case PREDICTION_FRAME_TO_TOP:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_FRAME_TO_BOTOM:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_TOP_TO_TOP:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_TOP_TO_BOTOM:
		r_offset = in->width * y + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_BOTOM_TO_TOP:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * y + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_BOTOM_TO_BOTOM:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	default:
		return;
	}

	r = in->y + r_offset;
	p = out->y + p_offset;

	xh = xv & 1;
	yh = yv & 1;
	xiv = xv >> 1;
	yiv = yv >> 1;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);

	if(chroma_format != 3){
		width >>= 1;
		xv /= 2;
		x /= 2;
		r_offset -= x;
		p_offset -= x;
	}
	if(chroma_format == 1){
		height >>= 1;
		yv /= 2;
		y /= 2;
		r_offset -= in->width * y;
		p_offset -= in->width * y;
	}

	r = in->u + r_offset;
	p = out->u + p_offset;

	xh = xv & 1;
	yh = yv & 1;
	xiv = xv >> 1;
	yiv = yv >> 1;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);

	r = in->v + r_offset;
	p = out->v + p_offset;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);
}

static void prediction_w16_hh_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<16;x++){
			w = in[x] + in[x+1] + in[x+in_step] + in[x+in_step+1] + 2;
			out[x] = (w >> 2);
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w16_hh_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<16;x++){
			w = in[x] + in[x+1] + in[x+in_step] + in[x+in_step+1] + 2;
			w >>= 2;
			w += out[x] + 1;
			out[x] = (w >> 1);
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w16_fh_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<16;x++){
			w = in[x] + in[x+in_step] + 1;
			out[x] = (w >> 1);
		}
		in += in_step;
		out += out_step;
	}
}
	
static void prediction_w16_fh_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<16;x++){
			w = in[x] + in[x+in_step] + 1;
			w >>= 1;
			w += out[x] + 1;
			out[x] = (w >> 1);
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w16_hf_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<16;x++){
			w = in[x] + in[x+1] + 1;
			out[x] = (w >> 1);
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w16_hf_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<16;x++){
			w = in[x] + in[x+1] + 1;
			w >>= 1;
			w += out[x] + 1;
			out[x] = (w >> 1);
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w16_ff_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int y;

	for(y=0;y<height;y++){
		memcpy(out, in, 16);
		in += in_step;
		out += out_step;
	}
}

static void prediction_w16_ff_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<16;x++){
			w = in[x] + out[x] + 1;
			out[x] = w >> 1;
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w8_hh_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<8;x++){
			w = in[x] + in[x+1] + in[x+in_step] + in[x+in_step+1] + 2;
			out[x] = (w >> 2);
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w8_hh_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<8;x++){
			w = in[x] + in[x+1] + in[x+in_step] + in[x+in_step+1] + 2;
			w >>= 2;
			w += out[x] + 1;
			out[x] = (w >> 1);
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w8_fh_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<8;x++){
			w = in[x] + in[x+in_step] + 1;
			out[x] = (w >> 1);
		}
		in += in_step;
		out += out_step;
	}
}
	
static void prediction_w8_fh_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<8;x++){
			w = in[x] + in[x+in_step] + 1;
			w >>= 1;
			w += out[x] + 1;
			out[x] = (w >> 1);
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w8_hf_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<8;x++){
			w = in[x] + in[x+1] + 1;
			out[x] = (w >> 1);
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w8_hf_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<8;x++){
			w = in[x] + in[x+1] + 1;
			w >>= 1;
			w += out[x] + 1;
			out[x] = (w >> 1);
		}
		in += in_step;
		out += out_step;
	}
}

static void prediction_w8_ff_1st(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int y;

	for(y=0;y<height;y++){
		memcpy(out, in, 8);
		in += in_step;
		out += out_step;
	}
}

static void prediction_w8_ff_2nd(unsigned char *in, unsigned char *out, int in_step, int out_step, int height)
{
	int w;
	int x, y;

	for(y=0;y<height;y++){
		for(x=0;x<8;x++){
			w = in[x] + out[x] + 1;
			out[x] = w >> 1;
		}
		in += in_step;
		out += out_step;
	}
}

typedef void (* STD_MC_CORE)(unsigned char *, unsigned char *, int, int, int); 

void prediction(FRAME *in, FRAME *out, int type, int xv, int yv, int x, int y, int first, int chroma_format)
{
	int r_offset, p_offset;
	int r_step, p_step;
	int width, height;
	int xh, yh;
	int xiv, yiv;
	unsigned char *r, *p;

	static STD_MC_CORE table[2][2][2][2] = {
		{/* width - 8 */
			{ /* horizontal full */
				{ /* vertical full */
					prediction_w8_ff_2nd,
					prediction_w8_ff_1st,
				},
				{ /* vertical half */
					prediction_w8_fh_2nd,
					prediction_w8_fh_1st,
				},
			},
			{ /* horizontal half */
				{ /* vertical full */
					prediction_w8_hf_2nd,
					prediction_w8_hf_1st,
				},
				{ /* vertical half */
					prediction_w8_hh_2nd,
					prediction_w8_hh_1st,
				},
			},
		},
		{/* width - 16 */
			{ /* horizontal full */
				{ /* vertical full */
					prediction_w16_ff_2nd,
					prediction_w16_ff_1st,
				},
				{ /* vertical half */
					prediction_w16_fh_2nd,
					prediction_w16_fh_1st,
				},
			},
			{ /* horizontal half */
				{ /* vertical full */
					prediction_w16_hf_2nd,
					prediction_w16_hf_1st,
				},
				{ /* vertical half */
					prediction_w16_hh_2nd,
					prediction_w16_hh_1st,
				},
			},
		},
	};

	switch(type){
	case PREDICTION_FRAME_TO_FRAME:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width;
		p_step = in->width;
		width = 16;
		height = 16;
		break;
	case PREDICTION_FRAME_TO_TOP:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_FRAME_TO_BOTOM:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_TOP_TO_TOP:
		r_offset = in->width * y + x;
		p_offset = in->width * y + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_TOP_TO_BOTOM:
		r_offset = in->width * y + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_BOTOM_TO_TOP:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * y + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	case PREDICTION_BOTOM_TO_BOTOM:
		r_offset = in->width * (y+1) + x;
		p_offset = in->width * (y+1) + x;
		r_step = in->width * 2;
		p_step = in->width * 2;
		width = 16;
		height = 8;
		break;
	default:
		return;
	}

	r = in->y + r_offset;
	p = out->y + p_offset;

	xh = xv & 1;
	yh = yv & 1;
	xiv = xv >> 1;
	yiv = yv >> 1;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);

	if(chroma_format != 3){
		width >>= 1;
		xv /= 2;
		x /= 2;
		r_offset -= x;
		p_offset -= x;
	}
	if(chroma_format == 1){
		height >>= 1;
		yv /= 2;
		y /= 2;
		r_offset -= in->width * y;
		p_offset -= in->width * y;
	}

	r = in->u + r_offset;
	p = out->u + p_offset;

	xh = xv & 1;
	yh = yv & 1;
	xiv = xv >> 1;
	yiv = yv >> 1;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);

	r = in->v + r_offset;
	p = out->v + p_offset;

	r += yiv * r_step + xiv;

	table[width>>4][xh][yh][first](r, p, r_step, p_step, height);
}
