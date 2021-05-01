/*******************************************************************
                          resize module
 *******************************************************************/
#define RESIZE_C
#include "resize.h"

#include <math.h>
#include <stdlib.h>
#include <float.h>

#ifndef PI
#define PI (atan(1)*4)
#endif

/* grobal */
void resize(FRAME *in, FRAME *out, RESIZE_PARAMETER *prm);
RESIZE_PARAMETER *create_resize_parameter(SEQUENCE_HEADER *seq, M2V_CONFIG *cfg);
RESIZE_PARAMETER *create_force_resize_parameter(SEQUENCE_HEADER *seq, int width, int height);
void release_resize_parameter(RESIZE_PARAMETER *prm);

/* local */
static void setup_interpolation_parameter(int source_length, int result_length, COMPONENT_RESIZE_PARAMETER *out);
static void setup_decimation_parameter(int source_length, int result_length, COMPONENT_RESIZE_PARAMETER *out);
static double lanczos3_weight(double phase);

static void setup_crop_parameter(int source_length, COMPONENT_RESIZE_PARAMETER *r);

static void component_resize(unsigned char *in, unsigned char *out, COMPONENT_RESIZE_PARAMETER *prm);

/*-----------------------------------------------------------------*/
void resize(FRAME *in, FRAME *out, RESIZE_PARAMETER *prm)
{
	component_resize(in->y, out->y, &(prm->l));
	component_resize(in->u, out->u, &(prm->c));
	component_resize(in->v, out->v, &(prm->c));
}

/*-----------------------------------------------------------------*/
RESIZE_PARAMETER *create_resize_parameter(SEQUENCE_HEADER *seq, M2V_CONFIG *cfg)
{
	RESIZE_PARAMETER *r;

	int n;
	int src_width[2];
	
	int chroma_format;
	
	if(cfg->aspect_ratio == M2V_CONFIG_IGNORE_ASPECT_RATIO){
		return NULL;
	}

	r = (RESIZE_PARAMETER *)malloc(sizeof(RESIZE_PARAMETER));
	if(r == NULL){
		return NULL;
	}

	if(seq->has_sequence_display_extension){
		if(    (seq->sd.display_h_size == 0)
		    || (seq->sd.display_v_size == 0)
		    || (seq->sd.display_h_size > seq->h_size)
		    || (seq->sd.display_v_size > seq->v_size) ) {
			seq->sd.display_h_size = seq->h_size;
			seq->sd.display_v_size = seq->v_size;
		}
	}
	
	if(seq->has_sequence_display_extension){
		r->l.height = seq->sd.display_v_size;
	}else{
		r->l.height = seq->orig_v_size;
	}
	r->c.height = r->l.height;

	if(seq->has_sequence_display_extension){
		src_width[0] = seq->sd.display_h_size;
	}else{
		src_width[0] = seq->orig_h_size;
	}

	if(seq->has_sequence_extension){
		switch(seq->aspect_ratio){
		case 2: /* 4:3 */
			r->l.width = r->l.height * 4 / 3;
			break;
		case 3: /* 16:9 */
			r->l.width = r->l.height * 16 / 9;
			break;
		case 4:
			r->l.width = r->l.height * 221 / 100;
			break;
		default:
			r->l.width = src_width[0];
		}
		chroma_format = seq->se.chroma_format;
	}else{
		switch(seq->aspect_ratio){
		case 2: /* 0.6735 */
			r->l.width = src_width[0] * 10000 / 6735;
			break;
		case 3: /* 0.7031 */
			r->l.width = src_width[0] * 10000 / 7031;
			break;
		case 4: /* 0.7615 */
			r->l.width = src_width[0] * 10000 / 7615;
			break;
		case 5: /* 0.8055 */
			r->l.width = src_width[0] * 10000 / 8055;
			break;
		case 6: /* 0.8437 */
			r->l.width = src_width[0] * 10000 / 8437;
			break;
		case 7: /* 0.8935 */
			r->l.width = src_width[0] * 10000 / 8935;
			break;
		case 8: /* 0.9815 */
			r->l.width = src_width[0] * 10000 / 9815;
			break;
		case 9: /* 54:59 PAL */
			r->l.width = src_width[0] * 59 / 54;
			break; 
		case 10: /* 1.0255 */
			r->l.width = src_width[0] * 10000 / 10255;
			break;
		case 11: /* 1.0695 */
			r->l.width = src_width[0] * 10000 / 10695;
			break;
		case 12: /* 11:10 NTSC */
			r->l.width = src_width[0] * 10 / 11;
			break;
		case 13: /* 1.1575 */
			r->l.width = src_width[0] * 10000 / 11575;
			break;
		case 14: /* 1.2015 */
			r->l.width = src_width[0] * 10000 / 12015;
			break;
		default:
			r->l.width = src_width[0];
		}
		chroma_format = 1;
	}				
	
	if(r->l.width == seq->orig_h_size){
		free(r);
		return NULL;
	}

	/* width is restricted to the multiple of 2. */
	r->l.width += 1;
	r->l.width &= 0xfffffffe;

	if(chroma_format == 3){ /* YUV 444 */
		r->c.width = r->l.width;
		src_width[1] = src_width[0];
	}else{
		r->c.width = r->l.width/2;
		src_width[1] = src_width[0]/2;
	}

	r->l.in_step = seq->h_size;
	r->c.in_step = r->l.in_step;

	r->l.out_step = (r->l.width + 15) & 0xfffffff0;
	r->c.out_step = r->l.out_step;

	if(seq->has_sequence_display_extension){
		n = (seq->orig_v_size - seq->sd.display_v_size) / 2;
		r->l.in_offset = r->l.in_step * n;
		r->c.in_offset = r->l.in_offset;

		n = (seq->orig_h_size - seq->sd.display_h_size) / 2;
		r->l.in_offset += n;
		if(chroma_format == 3){
			r->c.in_offset += n;
		}else{
			r->c.in_offset += n/2;
		}
	}else{
		r->l.in_offset = 0;
		r->c.in_offset = 0;
	}

	r->l.out_offset = 0;
	
	if(chroma_format == 1){ /* YUV 420 */
		r->c.in_offset += r->l.in_step/2;
		r->c.out_offset = r->l.out_step / 2;
	}else{
		r->c.out_offset = 0;
	}
	
	if(r->l.width < src_width[0]){
		setup_decimation_parameter(src_width[0], r->l.width, &(r->l));
		setup_decimation_parameter(src_width[1], r->c.width, &(r->c));
	}else if(r->l.width == src_width[0]){
		setup_crop_parameter(src_width[0], &(r->l));
		setup_crop_parameter(src_width[1], &(r->c));
	}else{
		setup_interpolation_parameter(src_width[0], r->l.width, &(r->l));
		setup_interpolation_parameter(src_width[1], r->c.width, &(r->c));
	}

	return r;
}

/*-----------------------------------------------------------------*/
RESIZE_PARAMETER *create_force_resize_parameter(SEQUENCE_HEADER *seq, int width, int height)
{
	RESIZE_PARAMETER *r;

	int n;
	int src_width[2];

	int chroma_format;

	r = (RESIZE_PARAMETER *)malloc(sizeof(RESIZE_PARAMETER));
	if(r == NULL){
		return NULL;
	}

	if(seq->has_sequence_display_extension){
		if(    (seq->sd.display_h_size == 0)
		    || (seq->sd.display_v_size == 0)
		    || (seq->sd.display_h_size > seq->h_size)
		    || (seq->sd.display_v_size > seq->v_size) ) {
			seq->sd.display_h_size = seq->h_size;
			seq->sd.display_v_size = seq->v_size;
		}
	}
	
	if(seq->has_sequence_display_extension){
		r->l.height = seq->sd.display_v_size;
	}else{
		r->l.height = seq->orig_v_size;
	}
	if(height < r->l.height){
		r->l.height = height;
	}
	r->c.height = r->l.height;
	
	if(seq->has_sequence_display_extension){
		src_width[0] = seq->sd.display_h_size;
	}else{
		src_width[0] = seq->orig_h_size;
	}
	r->l.width = width;
	if(r->l.width == seq->orig_h_size){
		free(r);
		return NULL;
	}

	/* width is restricted to the multiple of 2. */
	r->l.width += 1;
	r->l.width &= 0xfffffffe;

	if(seq->has_sequence_extension){
		chroma_format = seq->se.chroma_format;
	}else{
		chroma_format = 1;
	}

	if(chroma_format == 3){ /* YUV 444 */
		r->c.width = r->l.width;
		src_width[1] = src_width[0];
	}else{
		r->c.width = r->l.width/2;
		src_width[1] = src_width[0]/2;
	}
	
	r->l.in_step = seq->h_size;
	r->c.in_step = r->l.in_step;
	
	r->l.out_step = (r->l.width + 15) & 0xfffffff0;
	r->c.out_step = r->l.out_step;

	if(seq->has_sequence_display_extension){
		n = (seq->orig_v_size - seq->sd.display_v_size) / 2;
		r->l.in_offset = r->l.in_step * n;
		r->c.in_offset = r->l.in_offset;

		n = (seq->orig_h_size - seq->sd.display_h_size) / 2;
		r->l.in_offset += n;
		if(chroma_format == 3){
			r->c.in_offset += n;
		}else{
			r->c.in_offset += n/2;
		}
	}else{
		r->l.in_offset = 0;
		r->c.in_offset = 0;
	}

	r->l.out_offset = 0;
	
	if(chroma_format == 1){ /* YUV 420 */
		r->c.in_offset += r->l.in_step/2;
		r->c.out_offset = r->l.out_step / 2;
	}else{
		r->c.out_offset = 0;
	}
	
	if(r->l.width < src_width[0]){
		setup_decimation_parameter(src_width[0], r->l.width, &(r->l));
		setup_decimation_parameter(src_width[1], r->c.width, &(r->c));
	}else if(r->l.width == src_width[0]){
		setup_crop_parameter(src_width[0], &(r->l));
		setup_crop_parameter(src_width[1], &(r->c));
	}else{
		setup_interpolation_parameter(src_width[0], r->l.width, &(r->l));
		setup_interpolation_parameter(src_width[1], r->c.width, &(r->c));
	}

	return r;
}

/*-----------------------------------------------------------------*/
void release_resize_parameter(RESIZE_PARAMETER *prm)
{
	int i;
	
	if(prm == NULL){
		return;
	}

	for(i=0;i<prm->l.length;i++){
		free(prm->l.weight[i]);
		free(prm->l.index[i]);
	}

	free(prm->l.index);
	free(prm->l.weight);

	for(i=0;i<prm->c.length;i++){
		free(prm->c.weight[i]);
		free(prm->c.index[i]);
	}

	free(prm->c.index);
	free(prm->c.weight);

	free(prm);
}

/*-----------------------------------------------------------------*/
static void setup_interpolation_parameter(int source_length, int result_length, COMPONENT_RESIZE_PARAMETER *out)
{
	int i,j,n;
	double *work;
	double  sum;
	double  pos;

	out->length = result_length;
	out->index = (int **)malloc(sizeof(int *)*out->length);
	out->weight = (int **)malloc(sizeof(int *)*out->length);
	out->tap = 6;

	for(i=0;i<result_length;i++){
		out->weight[i] = (int *)malloc(sizeof(int)*out->tap);
		out->index[i] = (int *)malloc(sizeof(int)*out->tap);
	}
	work = (double *)malloc(sizeof(double)*out->tap);

	__asm {emms};

	for(i=0;i<result_length;i++){
		pos = (i+0.5)*source_length;
		pos /= result_length;
		n = floor(pos-2.5);
		pos = (n+0.5-pos);
		sum = 0;
		for(j=0;j<out->tap;j++){
			if(n < 0){
				out->index[i][j] = 0;
			}else if(n >= source_length){
				out->index[i][j] = source_length-1;
			}else{
				out->index[i][j] = n;
			}
			work[j] = lanczos3_weight(pos);
			sum += work[j];
			pos += 1;
			n += 1;
		}

		for(j=0;j<out->tap;j++){
			out->weight[i][j] = (int)((work[j] / sum) * (1<<16));
		}
	}

	free(work);
}

/*-----------------------------------------------------------------*/
static void setup_decimation_parameter(int source_length, int result_length, COMPONENT_RESIZE_PARAMETER *out)
{
	int i,j,n;
	double *work;
	double  sum;
	double  pos, phase;

	out->length = result_length;
	out->weight = (int **)malloc(sizeof(int *)*out->length);
	out->index = (int **)malloc(sizeof(int *)*out->length);
	
	__asm {emms};

	out->tap = (6*(source_length)+(result_length-1)) / result_length;

	if((source_length % result_length) == 0){
		out->tap -= 1;
	}

	for(i=0;i<result_length;i++){
		out->weight[i] = (int *)malloc(sizeof(int)*out->tap);
		out->index[i] = (int *)malloc(sizeof(int)*out->tap);
	}
	work = (double *)malloc(sizeof(double)*out->tap);

	for(i=0;i<result_length;i++){
		pos = (i-3+0.5)*source_length/result_length + 0.5;
		n = floor(pos);
		sum = 0;
		for(j=0;j<out->tap;j++){
			phase = (n+0.5)*result_length;
			phase /= source_length;
			phase -= (i+0.5);
			if(n < 0){
				out->index[i][j] = 0;
			}else if(n >= source_length){
				out->index[i][j] = source_length-1;
			}else{
				out->index[i][j] = n;
			}
			work[j] = lanczos3_weight(phase);
			sum += work[j];
			n += 1;
		}

		for(j=0;j<out->tap;j++){
			out->weight[i][j] = (int)((work[j] / sum) * (1<<16));
		}
	}

	free(work);
}

/*-----------------------------------------------------------------*/
static double lanczos3_weight(double phase)
{
	double ret;
	
	if(fabs(phase) < DBL_EPSILON){
		return 1.0;
	}

	if(fabs(phase) >= 3.0){
		return 0.0;
	}

	ret = sin(PI*phase)*sin(PI*phase/3)/(PI*PI*phase*phase/3);

	return ret;
}

/*-----------------------------------------------------------------*/
static void setup_crop_parameter(int result_length, COMPONENT_RESIZE_PARAMETER *out)
{
	int i;

	out->length = result_length;
	out->index = (int **)malloc(sizeof(int)*out->length);
	out->weight = (int **)malloc(sizeof(int *)*out->length);
	out->tap = 1;

	for(i=0;i<result_length;i++){
		out->weight[i] = (int *)malloc(sizeof(int));
		out->index[i] = (int *)malloc(sizeof(int));
	}

	for(i=0;i<result_length;i++){
		out->index[i][0] = i;
		out->weight[i][0] = 1<<16;
	}

	return;
}

/*-----------------------------------------------------------------*/
static void component_resize(unsigned char *in, unsigned char *out, COMPONENT_RESIZE_PARAMETER *prm)
{
	int x,y;
	int i;
	int w;

	in += prm->in_offset;
	out += prm->out_offset;

	for(y=0;y<prm->height;y++){
		for(x=0;x<prm->width;x++){
			w = 0;
			for(i=0;i<prm->tap;i++){
				w += in[prm->index[x][i]] * prm->weight[x][i];
			}
			w += 32768;
			out[x] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+(w>>16)];
		}
		out += prm->out_step;
		in += prm->in_step;
	}
}

