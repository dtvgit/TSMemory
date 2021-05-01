/*******************************************************************
                  YUV frame treating module
 *******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frame.h"

const unsigned char uchar_clip_table[1024] = {
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  0,  0,  0,  0,  0,  0,  0,
	  0,  1,  2,  3,  4,  5,  6,  7,
	  8,  9, 10, 11, 12, 13, 14, 15,
	 16, 17, 18, 19, 20, 21, 22, 23,
	 24, 25, 26, 27, 28, 29, 30, 31,
	 32, 33, 34, 35, 36, 37, 38, 39,
	 40, 41, 42, 43, 44, 45, 46, 47,
	 48, 49, 50, 51, 52, 53, 54, 55,
	 56, 57, 58, 59, 60, 61, 62, 63,
	 64, 65, 66, 67, 68, 69, 70, 71,
	 72, 73, 74, 75, 76, 77, 78, 79,
	 80, 81, 82, 83, 84, 85, 86, 87,
	 88, 89, 90, 91, 92, 93, 94, 95,
	 96, 97, 98, 99,100,101,102,103,
	104,105,106,107,108,109,110,111,
	112,113,114,115,116,117,118,119,
	120,121,122,123,124,125,126,127,
	128,129,130,131,132,133,134,135,
	136,137,138,139,140,141,142,143,
	144,145,146,147,148,149,150,151,
	152,153,154,155,156,157,158,159,
	160,161,162,163,164,165,166,167,
	168,169,170,171,172,173,174,175,
	176,177,178,179,180,181,182,183,
	184,185,186,187,188,189,190,191,
	192,193,194,195,196,197,198,199,
	200,201,202,203,204,205,206,207,
	208,209,210,211,212,213,214,215,
	216,217,218,219,220,221,222,223,
	224,225,226,227,228,229,230,231,
	232,233,234,235,236,237,238,239,
	240,241,242,243,244,245,246,247,
	248,249,250,251,252,253,254,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
	255,255,255,255,255,255,255,255,
};

static void chroma420i_to_422(unsigned char *data, int width, int height);
static void chroma420p_to_422(unsigned char *data, int width, int height);

extern void __stdcall chroma420i_to_422_mmx(unsigned char *data, int width, int height);
extern void __stdcall chroma420p_to_422_mmx(unsigned char *data, int width, int height);

extern void __stdcall chroma420i_to_422_sse2(unsigned char *data, int width, int height);
extern void __stdcall chroma420p_to_422_sse2(unsigned char *data, int width, int height);

void copy_frame(FRAME *in, FRAME *out)
{
	if ((in == NULL) || (out == NULL)) {
		return;
	}

	memcpy(out->y, in->y, in->width * in->height);
	memcpy(out->u, in->u, in->width * in->height);
	memcpy(out->v, in->v, in->width * in->height);
}

static void copy_field(unsigned char *dst, unsigned char *src, int width, int height)
{
	int y;

	for (y=0;y<(height/2);y++) {
		memcpy(dst, src, width);
		src += width*2;
		dst += width*2;
	}
}

void copy_2nd_field(FRAME *frame, int picture_structure)
{
	if (picture_structure == 1) {
		/* copy top field to bottom */
		copy_field(frame->y + frame->width, frame->y, frame->width, frame->height);
		copy_field(frame->u + frame->width, frame->u, frame->width, frame->height);
		copy_field(frame->v + frame->width, frame->v, frame->width, frame->height);
	} else {
		/* copy bottom field to top */
		copy_field(frame->y, frame->y + frame->width, frame->width, frame->height);
		copy_field(frame->u, frame->u + frame->width, frame->width, frame->height);
		copy_field(frame->v, frame->v + frame->width, frame->width, frame->height);
	}
}

void __stdcall yuv444_to_bgr(FRAME *top, FRAME *bottom, unsigned char *out, BGR_CONVERSION_PARAMETER *prm)
{
	int i, j;
	unsigned char *yt, *ut, *vt;
	unsigned char *yb, *ub, *vb;
	unsigned char *r, *g, *b;

	yt = top->y;
	ut = top->u;
	vt = top->v;

	yb = bottom->y + prm->prm.in_step;
	ub = bottom->u + prm->prm.in_step;
	vb = bottom->v + prm->prm.in_step;

	b = out;
	g = out+1;
	r = out+2;

	for(i=0;i<prm->prm.height/2;i++){
		for(j=0;j<prm->prm.width;j++){
			b[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->bu * (((int)ut[j]) - 128)) >> 16)];
			g[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->gu * (((int)ut[j]) - 128) + prm->gv * (((int)vt[j]) - 128)) >> 16)];
			r[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->rv * (((int)vt[j]) - 128)) >> 16)];
		}
		yt += prm->prm.in_step*2;
		ut += prm->prm.in_step*2;
		vt += prm->prm.in_step*2;
		b += prm->prm.out_step;
		g += prm->prm.out_step;
		r += prm->prm.out_step;
		
		for(j=0;j<prm->prm.width;j++){
			b[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yb[j] - prm->yo) + prm->bu * (((int)ub[j]) - 128)) >> 16)];
			g[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yb[j] - prm->yo) + prm->gu * (((int)ub[j]) - 128) + prm->gv * (((int)vb[j]) - 128)) >> 16)];
			r[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yb[j] - prm->yo) + prm->rv * (((int)vb[j]) - 128)) >> 16)];
		}
		yb += prm->prm.in_step*2;
		ub += prm->prm.in_step*2;
		vb += prm->prm.in_step*2;
		b += prm->prm.out_step;
		g += prm->prm.out_step;
		r += prm->prm.out_step;
	}

	if(prm->prm.height%2){
		for(j=0;j<prm->prm.width;j++){
			b[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->bu * (((int)ut[j]) - 128)) >> 16)];
			g[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->gu * (((int)ut[j]) - 128) + prm->gv * (((int)vt[j]) - 128)) >> 16)];
			r[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->rv * (((int)vt[j]) - 128)) >> 16)];
		}
	}
}

void __stdcall yuv422_to_bgr(FRAME *top, FRAME *bottom, unsigned char *out, BGR_CONVERSION_PARAMETER *prm)
{
	int i, j;
	unsigned char *yt, *ut, *vt;
	unsigned char *yb, *ub, *vb;
	unsigned char *r, *g, *b;

	yt = top->y;
	ut = top->u + prm->prm.c_offset;
	vt = top->v + prm->prm.c_offset;

	yb = bottom->y + prm->prm.in_step;
	ub = bottom->u + prm->prm.in_step + prm->prm.c_offset;
	vb = bottom->v + prm->prm.in_step + prm->prm.c_offset;

	b = out;
	g = out+1;
	r = out+2;

	for(i=0;i<prm->prm.height/2;i++){
		for(j=0;j<prm->prm.width;j++){
			b[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->bu * (((int)ut[j/2]) - 128)) >> 16)];
			g[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->gu * (((int)ut[j/2]) - 128) + prm->gv * (((int)vt[j/2]) - 128)) >> 16)];
			r[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->rv * (((int)vt[j/2]) - 128)) >> 16)];
		}
		yt += prm->prm.in_step*2;
		ut += prm->prm.in_step*2;
		vt += prm->prm.in_step*2;
		b += prm->prm.out_step;
		g += prm->prm.out_step;
		r += prm->prm.out_step;
		
		for(j=0;j<prm->prm.width;j++){
			b[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yb[j] - prm->yo) + prm->bu * (((int)ub[j/2]) - 128)) >> 16)];
			g[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yb[j] - prm->yo) + prm->gu * (((int)ub[j/2]) - 128) + prm->gv * (((int)vb[j/2]) - 128)) >> 16)];
			r[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yb[j] - prm->yo) + prm->rv * (((int)vb[j/2]) - 128)) >> 16)];
		}
		yb += prm->prm.in_step*2;
		ub += prm->prm.in_step*2;
		vb += prm->prm.in_step*2;
		b += prm->prm.out_step;
		g += prm->prm.out_step;
		r += prm->prm.out_step;
	}

	if(prm->prm.height%2){
		for(j=0;j<prm->prm.width;j++){
			b[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->bu * (((int)ut[j/2]) - 128)) >> 16)];
			g[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->gu * (((int)ut[j/2]) - 128) + prm->gv * (((int)vt[j/2]) - 128)) >> 16)];
			r[j*3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(( prm->yg * ((int)yt[j] - prm->yo) + prm->rv * (((int)vt[j/2]) - 128)) >> 16)];
		}
	}
}

void __stdcall yuv444_to_yuy2(FRAME *top, FRAME *bottom, unsigned char *out, CONVERSION_PARAMETER *prm)
{
	int i, j;
	unsigned char *yt, *ut, *vt;
	unsigned char *yb, *ub, *vb;

	yt = top->y;
	ut = top->u;
	vt = top->v;

	yb = bottom->y + prm->in_step;
	ub = bottom->u + prm->in_step;
	vb = bottom->v + prm->in_step;

	for(i=0;i<prm->height/2;i++){
		out[0] = yt[0];
		out[1] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((ut[0]*3+ut[1]+2)>>2)];
		out[2] = yt[0+1];
		out[3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((vt[0]*3+vt[1]+2)>>2)];
		for(j=2;j<prm->width-2;j+=2){
			out[j*2+0] = yt[j];
			out[j*2+1] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((ut[j-1]+ut[j]*2+ut[j+1]+2)>>2)];
			out[j*2+2] = yt[j+1];
			out[j*2+3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((vt[j-1]+vt[j]*2+vt[j+1]+2)>>2)];
		}
		out[j*2+0] = yt[j];
		out[j*2+1] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((ut[j-1]+ut[j]*3+2)>>2)];
		out[j*2+2] = yt[j+1];
		out[j*2+3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((vt[j-1]+vt[j]*3+2)>>2)];
		yt += prm->in_step*2;
		ut += prm->in_step*2;
		vt += prm->in_step*2;
		out += prm->out_step;

		
		out[0] = yb[0];
		out[1] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((ub[0]+ub[1]*3+2)>>2)];
		out[2] = yb[1];
		out[3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((vb[0]+vb[1]*3+2)>>2)];
		for(j=2;j<prm->width-2;j+=2){
			out[j*2+0] = yb[j];
			out[j*2+1] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((ub[j-1]+ub[j]*2+ub[j+1]+2)>>2)];
			out[j*2+2] = yb[j+1];
			out[j*2+3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((vb[j-1]+vb[j]*2+vb[j+1]+2)>>2)];
		}
		out[j*2+0] = yb[j];
		out[j*2+1] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((ub[j-1]+ub[j]*3+2)>>2)];
		out[j*2+2] = yb[j+1];
		out[j*2+3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((vb[j-1]+vb[j]*3+2)>>2)];
		yb += prm->in_step*2;
		ub += prm->in_step*2;
		vb += prm->in_step*2;
		out += prm->out_step;
	}

	if(prm->height%2){
		out[0] = yt[0];
		out[1] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((ut[0]*3+ut[1]+2)>>2)];
		out[2] = yt[0+1];
		out[3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((vt[0]*3+vt[1]+2)>>2)];
		for(j=2;j<prm->width-2;j+=2){
			out[j*2+0] = yt[j];
			out[j*2+1] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((ut[j-1]+ut[j]*2+ut[j+1]+2)>>2)];
			out[j*2+2] = yt[j+1];
			out[j*2+3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((vt[j-1]+vt[j]*2+vt[j+1]+2)>>2)];
		}
		out[j*2+0] = yt[j];
		out[j*2+1] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((ut[j-1]+ut[j]*3+2)>>2)];
		out[j*2+2] = yt[j+1];
		out[j*2+3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+((vt[j-1]+vt[j]*3+2)>>2)];
	}
}

void __stdcall yuv422_to_yuy2(FRAME *top, FRAME *bottom, unsigned char *out, CONVERSION_PARAMETER *prm)
{
	int i, j;
	unsigned char *yt, *ut, *vt;
	unsigned char *yb, *ub, *vb;

	yt = top->y;
	ut = top->u + prm->c_offset;
	vt = top->v + prm->c_offset;

	yb = bottom->y + prm->in_step;
	ub = bottom->u + prm->in_step + prm->c_offset;
	vb = bottom->v + prm->in_step + prm->c_offset;

	for(i=0;i<prm->height/2;i++){
		for(j=0;j<prm->width;j+=2){
			out[j*2+0] = yt[j];
			out[j*2+1] = ut[j/2];
			out[j*2+2] = yt[j+1];
			out[j*2+3] = vt[j/2];
		}
		yt += prm->in_step*2;
		ut += prm->in_step*2;
		vt += prm->in_step*2;
		out += prm->out_step;

		for(j=0;j<prm->width;j+=2){
			out[j*2+0] = yb[j];
			out[j*2+1] = ub[j/2];
			out[j*2+2] = yb[j+1];
			out[j*2+3] = vb[j/2];
		}
		yb += prm->in_step*2;
		ub += prm->in_step*2;
		vb += prm->in_step*2;
		out += prm->out_step;
	}
}

void __stdcall yuy2_convert(unsigned char *yuy2, int step, int height, YUY2_CONVERSION_PARAMETER *prm)
{
	int i,j;
	int w;
	int work;
	int ya,yb,u,v;

	if(step>0){
		w = step/4;
	}else{
		w = (0-step)/4;
	}

	for(i=0;i<height;i++){
		for(j=0;j<w;j++){
			ya = yuy2[j*4+0];
			u  = yuy2[j*4+1];
			yb = yuy2[j*4+2];
			v  = yuy2[j*4+3];
			u -= 128;
			v -= 128;
			work = (u*prm->yu + v*prm->yv + 32768)>>16;
			yuy2[j*4+0] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(ya+work)];
			yuy2[j*4+1] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(((u*prm->uu+v*prm->uv+32768)>>16)+128)];
			yuy2[j*4+2] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(yb+work)];
			yuy2[j*4+3] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(((u*prm->vu+v*prm->vv+32768)>>16)+128)];
		}
		yuy2 += step;
	}
}

void __stdcall yuy2_convert_none(unsigned char *yuy2, int step, int height, YUY2_CONVERSION_PARAMETER *prm)
{
	// nothing to do
}

static void chroma420i_to_422(unsigned char *data, int width, int height)
{
	int x,y;
	int hw;

	unsigned char *d;
	unsigned char *s;
	
	hw = width >> 1;
	
	s = data;
	d = s + hw;

	/* copy top 2 line */
	memcpy(d, s, hw);
	d += width;
	memcpy(d, s+width, hw);
	d += width;

	/* interpolation */
	for(y=2;y<height-2;y+=4){
		for(x=0;x<hw;x++){
			d[0*width+x] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET + ((s[0*width+x]*5 + s[2*width+x]*3 + 4) >> 3)];
			d[1*width+x] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET + ((s[1*width+x]*7 + s[3*width+x]*1 + 4) >> 3)];
			d[2*width+x] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET + ((s[0*width+x]*1 + s[2*width+x]*7 + 4) >> 3)];
			d[3*width+x] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET + ((s[1*width+x]*3 + s[3*width+x]*5 + 4) >> 3)];
		}
		d += width*4;
		s += width*2;
	}

	/* copy bottom 2 line */
	memcpy(d, s, hw);
	d += width;
	s += width;
	memcpy(d, s, hw);
	
}

static void chroma420p_to_422(unsigned char *data, int width, int height)
{
	int x,y;
	int hw;
	unsigned char *d;
	unsigned char *s;

	hw = width >> 1;
	s = data;
	d = s + hw;

	/* copy top line */
	memcpy(d, s, hw);
	d += width;

	/* interpolation */
	for(y=1;y<height-1;y+=2){
		for(x=0;x<hw;x++){
			d[0*width+x] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET + ((s[0*width+x]*3 + s[1*width+x]*1 + 2) >> 2)];
			d[1*width+x] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET + ((s[0*width+x]*1 + s[1*width+x]*3 + 2) >> 2)];
		}
		d += width * 2;
		s += width;
	}

	/* copy bottom line */
	memcpy(d, s, hw);
}

void upsample_chroma_none(FRAME *p)
{
	return;
}

void upsample_chroma_420i(FRAME *p)
{
	chroma420i_to_422(p->u, p->width, p->height);
	chroma420i_to_422(p->v, p->width, p->height);
}

void upsample_chroma_420p(FRAME *p)
{
	chroma420p_to_422(p->u, p->width, p->height);
	chroma420p_to_422(p->v, p->width, p->height);
}

void upsample_chroma_420i_mmx(FRAME *p)
{
	chroma420i_to_422_mmx(p->u, p->width, p->height);
	chroma420i_to_422_mmx(p->v, p->width, p->height);
}

void upsample_chroma_420p_mmx(FRAME *p)
{
	chroma420p_to_422_mmx(p->u, p->width, p->height);
	chroma420p_to_422_mmx(p->v, p->width, p->height);
}

void upsample_chroma_420i_sse2(FRAME *p)
{
	chroma420i_to_422_sse2(p->u, p->width, p->height);
	chroma420i_to_422_sse2(p->v, p->width, p->height);
}

void upsample_chroma_420p_sse2(FRAME *p)
{
	chroma420p_to_422_sse2(p->u, p->width, p->height);
	chroma420p_to_422_sse2(p->v, p->width, p->height);
}
