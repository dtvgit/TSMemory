/*******************************************************************
                     macroblock layer interface
 *******************************************************************/
#include "macroblock.h"

typedef struct {
	int value;
	int length;
} BASIC_VLC_ELEMENT;

static int read_macroblock_mode(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt);
static int get_motion_code(VIDEO_STREAM *in);
static int get_dmvector(VIDEO_STREAM *in);
static void read_dual_prime_motion_vector(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt);
static void read_motion_vector(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt, int s);
static void read_dmv(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt, int s, int t);
static int get_motion_vector_predictor(VIDEO_STREAM *in, int predictor, int f_code, int half);
static void read_coded_block_mpeg2_420_intra(VIDEO_STREAM *in, MACROBLOCK *out);
static void read_coded_block_mpeg1_420_intra(VIDEO_STREAM *in, MACROBLOCK *out);
static void read_coded_block_mpeg2_422_intra(VIDEO_STREAM *in, MACROBLOCK *out);
static void read_coded_block_mpeg2_444_intra(VIDEO_STREAM *in, MACROBLOCK *out);
static void read_coded_block_mpeg2_420_nonintra_pattern(VIDEO_STREAM *in, MACROBLOCK *out);
static void read_coded_block_mpeg1_420_nonintra_pattern(VIDEO_STREAM *in, MACROBLOCK *out);
static void read_coded_block_mpeg2_422_nonintra_pattern(VIDEO_STREAM *in, MACROBLOCK *out);
static void read_coded_block_mpeg2_444_nonintra_pattern(VIDEO_STREAM *in, MACROBLOCK *out);
static void read_coded_block_420_nonintra_nopattern(VIDEO_STREAM *in, MACROBLOCK *out);
static void read_coded_block_422_nonintra_nopattern(VIDEO_STREAM *in, MACROBLOCK *out);
static void read_coded_block_444_nonintra_nopattern(VIDEO_STREAM *in, MACROBLOCK *out);
static int get_coded_block_pattern(VIDEO_STREAM *in);
static void read_spatial_temporal_weight(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt);

typedef void (* READ_CODED_BLOCK)(VIDEO_STREAM *, MACROBLOCK *);

int read_macroblock(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt)
{
	static const READ_CODED_BLOCK rcb[2][3][2][2] = {
		{    /* mpeg-2 */
			{    /* 420 */
				{   /* nonintra */
					read_coded_block_420_nonintra_nopattern,
					read_coded_block_mpeg2_420_nonintra_pattern,
				},{ /* intra */
					read_coded_block_mpeg2_420_intra,
					read_coded_block_mpeg2_420_intra,
				},
			}, { /* 422 */
				{   /* nonintra */
					read_coded_block_422_nonintra_nopattern,
					read_coded_block_mpeg2_422_nonintra_pattern,
				},{ /* intra */
					read_coded_block_mpeg2_422_intra,
					read_coded_block_mpeg2_422_intra,
				},
			}, { /* 444 */
				{   /* nonintra */
					read_coded_block_444_nonintra_nopattern,
					read_coded_block_mpeg2_444_nonintra_pattern,
				},{ /* intra */
					read_coded_block_mpeg2_444_intra,
					read_coded_block_mpeg2_444_intra,
				},
			},
		},{ /* mpeg-1 */
			{   /* 420 */
				{   /* nonintra */
					read_coded_block_420_nonintra_nopattern,
					read_coded_block_mpeg1_420_nonintra_pattern,
				},{ /* intra */
					read_coded_block_mpeg1_420_intra,
					read_coded_block_mpeg1_420_intra,
				},
			},{ /* 422 - dummy */
				{   /* nonintra */
					read_coded_block_420_nonintra_nopattern,
					read_coded_block_mpeg1_420_nonintra_pattern,
				},{ /* intra */
					read_coded_block_mpeg1_420_intra,
					read_coded_block_mpeg1_420_intra,
				},
			},{ /* 444 - dummy */
				{   /* nonintra */
					read_coded_block_420_nonintra_nopattern,
					read_coded_block_mpeg1_420_nonintra_pattern,
				},{ /* intra */
					read_coded_block_mpeg1_420_intra,
					read_coded_block_mpeg1_420_intra,
				},
			},
		},
	};
	
	if(!read_macroblock_mode(in, out, opt)){
		return 0;
	}

	if(out->macroblock_quant){
		out->quantizer_scale_code = vs_get_bits(in, 5);
	}

	if(out->macroblock_motion_forward || (out->macroblock_intra && opt->concealment_motion_vectors)){
		read_motion_vector(in, out, opt, 0);
	}

	if(out->macroblock_motion_backward){
		read_motion_vector(in, out, opt, 1);
	}

	if(out->macroblock_intra && opt->concealment_motion_vectors){
		vs_get_bits(in, 1); /* marker bit */
	}

	rcb[opt->mpeg1][opt->chroma_format-1][out->macroblock_intra][out->macroblock_pattern](in, out);

	return 1;
}

int macroblock_to_read_block_option(MACROBLOCK *in, READ_BLOCK_OPTION *out)
{
	out->macroblock_intra = in->macroblock_intra;
	out->dct_type = in->dct_type;

	if(in->macroblock_quant){
		out->qw[0] = out->qw_table[in->quantizer_scale_code][0];
		out->qw[1] = out->qw_table[in->quantizer_scale_code][1];
		out->qw[2] = out->qw_table[in->quantizer_scale_code][2];
		out->qw[3] = out->qw_table[in->quantizer_scale_code][3];
	}

	if(! in->macroblock_intra){
		reset_dc_dct_predictor(out);
	}
	
	return 1;
}

int get_macroblock_address_increment(VIDEO_STREAM *in)
{
	int r;
	int code;

	static const BASIC_VLC_ELEMENT table_a[] = {
		{9,7},{8,7},
		{7,5},{7,5},{7,5},{7,5},{6,5},{6,5},{6,5},{6,5},
		{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},
		{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},
		{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},{2,3},
		{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},
		{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},
		{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},
		{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},
		{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},
		{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},
		{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},
		{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},{1,1},
	};

	static const BASIC_VLC_ELEMENT table_b[] = {
		{33,11},{32,11},{31,11},{30,11},{29,11},{28,11},{27,11},{26,11},
		{25,11},{24,11},{23,11},{22,11},{21,10},{21,10},{20,10},{20,10},
		{19,10},{19,10},{18,10},{18,10},{17,10},{17,10},{16,10},{16,10},
		{15,8},{15,8},{15,8},{15,8},{15,8},{15,8},{15,8},{15,8},
		{14,8},{14,8},{14,8},{14,8},{14,8},{14,8},{14,8},{14,8},
		{13,8},{13,8},{13,8},{13,8},{13,8},{13,8},{13,8},{13,8},
		{12,8},{12,8},{12,8},{12,8},{12,8},{12,8},{12,8},{12,8},
		{11,8},{11,8},{11,8},{11,8},{11,8},{11,8},{11,8},{11,8},
		{10,8},{10,8},{10,8},{10,8},{10,8},{10,8},{10,8},{10,8},
	};

	r = 0;

	code = vs_read_bits(in, 11);

	while( (code = vs_read_bits(in, 11)) == 8 ){
		r += 33;
		vs_erase_bits(in, 11);
	}

	if(code > 95){
		code = (code >> 4) - 6;
		r += table_a[code].value;
		vs_erase_bits(in, table_a[code].length);
	}else if(code > 23){
		code -= 24;
		r += table_b[code].value;
		vs_erase_bits(in, table_b[code].length);
	}else{
		r = 0;
	}

	return r;
}

int macroblock_to_mc_parameter(MACROBLOCK *in, MC_PARAMETER *out)
{
	int r,s,t;

	out->macroblock_motion_forward = in->macroblock_motion_forward;
	out->macroblock_motion_backward = in->macroblock_motion_backward;

	out->prediction_type = in->prediction_type;

	for(r=0;r<2;r++){
		for(s=0;s<2;s++){
			for(t=0;t<2;t++){
				out->PMV[r][s][t] = in->PMV[r][s][t];
			}
			out->DMV[r][s] = in->DMV[r][s];
			out->motion_vertical_field_select[r][s] = in->motion_vertical_field_select[r][s];
		}
	}

	return 1;
}

int macroblock_to_error_concealment_mc_parameter(MACROBLOCK *in, MC_PARAMETER *out)
{
	int r,s,t;

	out->macroblock_motion_forward = in->macroblock_motion_forward;
	out->macroblock_motion_backward = in->macroblock_motion_backward;

	out->prediction_type = in->prediction_type;

	for(r=0;r<2;r++){
		for(s=0;s<2;s++){
			for(t=0;t<2;t++){
				out->PMV[r][s][t] = in->PMV[r][s][t];
			}
			out->DMV[r][s] = in->DMV[r][s];
			out->motion_vertical_field_select[r][s] = in->motion_vertical_field_select[r][s];
		}
	}

	if(in->macroblock_intra){
		out->macroblock_motion_forward = 1;
		out->prediction_type = PREDICTION_TYPE_FRAME_BASED;
		memset(out->PMV, 0, sizeof(out->PMV));
		if(out->picture_structure == 2){
			out->motion_vertical_field_select[0][0] = 1;
		}else{
			out->motion_vertical_field_select[0][0] = 0;
		}
	}

	return 1;
}

int reset_motion_vector_predictor(MACROBLOCK *p)
{
	int r,s,t;

	for(r=0;r<2;r++){
		for(s=0;s<2;s++){
			for(t=0;t<2;t++){
				p->PMV[r][s][t] = 0;
			}
		}
	}

	return 1;
}

static int read_macroblock_mode(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt)
{
	int code;

	if(!opt->read_macroblock_type(in, out)){
		return 0;
	}

	if(out->spatial_temporal_weight_code_flag){
		read_spatial_temporal_weight(in, out, opt);
	}

	if(out->macroblock_motion_forward || out->macroblock_motion_backward){
		if(opt->picture_structure == 3){
			if(! opt->frame_predictive_frame_dct){
				code = vs_get_bits(in, 2);
				switch(code){
				case 0:
					return 0;
				case 1:
					out->prediction_type = PREDICTION_TYPE_FIELD_BASED;
					if( (out->spatial_temporal_weight_class == 0) || (out->spatial_temporal_weight_class == 1) ){
						out->motion_vector_count = 2;
					}else{
						out->motion_vector_count = 1;
					}
					out->motion_vector_format = MOTION_VECTOR_FORMAT_FIELD;
					out->dual_prime_motion_vector = 0;
					break;
				case 2:
					out->prediction_type = PREDICTION_TYPE_FRAME_BASED;
					out->motion_vector_count = 1;
					out->motion_vector_format = MOTION_VECTOR_FORMAT_FRAME;
					out->dual_prime_motion_vector = 0;
					break;
				case 3:
					out->prediction_type = PREDICTION_TYPE_DUAL_PRIME;
					out->motion_vector_count = 1;
					out->motion_vector_format = MOTION_VECTOR_FORMAT_FIELD;
					out->dual_prime_motion_vector = 1;
				}
			}else{
				out->prediction_type = PREDICTION_TYPE_FRAME_BASED;
				out->motion_vector_count = 1;
				out->motion_vector_format = MOTION_VECTOR_FORMAT_FRAME;
				out->dual_prime_motion_vector = 0;
			}
		}else{
			code = vs_get_bits(in, 2);
			switch(code){
			case 0:
				return 0;
			case 1:
				out->prediction_type = PREDICTION_TYPE_FIELD_BASED;
				out->motion_vector_count = 1;
				out->motion_vector_format = MOTION_VECTOR_FORMAT_FIELD;
				out->dual_prime_motion_vector = 0;
				break;
			case 2:
				out->prediction_type = PREDICTION_TYPE_16x8_MC;
				out->motion_vector_count = 2;
				out->motion_vector_format = MOTION_VECTOR_FORMAT_FIELD;
				out->dual_prime_motion_vector = 0;
				break;
			case 3:
				out->prediction_type = PREDICTION_TYPE_DUAL_PRIME;
				out->motion_vector_count = 1;
				out->motion_vector_format = MOTION_VECTOR_FORMAT_FIELD;
				out->dual_prime_motion_vector = 1;
				break;
			}
		}
	}else if(out->macroblock_intra && opt->concealment_motion_vectors){
		out->motion_vector_count = 1;
		out->dual_prime_motion_vector = 0;
		if(opt->picture_structure == 3){
			out->prediction_type = PREDICTION_TYPE_FRAME_BASED;
			out->motion_vector_format = MOTION_VECTOR_FORMAT_FRAME;
		}else{
			out->prediction_type = PREDICTION_TYPE_FIELD_BASED;
			out->motion_vector_format = MOTION_VECTOR_FORMAT_FIELD;
		}
	}else{
		out->motion_vector_count = 1;
		out->dual_prime_motion_vector = 0;

		reset_motion_vector_predictor(out);
		
		switch(opt->picture_structure){
		case 1:
			out->prediction_type = PREDICTION_TYPE_FIELD_BASED;
			out->motion_vector_format = MOTION_VECTOR_FORMAT_FIELD;
			out->motion_vertical_field_select[0][0] = 0;
			break;
		case 2:
			out->prediction_type = PREDICTION_TYPE_FIELD_BASED;
			out->motion_vector_format = MOTION_VECTOR_FORMAT_FIELD;
			out->motion_vertical_field_select[0][0] = 1;
			break;
		case 3:
			out->prediction_type = PREDICTION_TYPE_FRAME_BASED;
			out->motion_vector_format = MOTION_VECTOR_FORMAT_FRAME;
			break;
		}
	}

	if(opt->frame_predictive_frame_dct){
		out->dct_type = 0;
	}else if( opt->picture_structure == 3 && (out->macroblock_intra || out->macroblock_pattern) ){
		out->dct_type = vs_get_bits(in, 1);
	}else{
		out->dct_type = 0;
	}

	return 1;
}

int read_macroblock_type_snr_scalability(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int code;
	
	code = vs_read_bits(in, 3);
	if( (code >> 2) == 1 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 1);
	}else if( (code >> 1) == 1){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 2);
	}else if(code == 1){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 3);
	}else{
		return 0;
	}

	return 1;
}

int read_macroblock_type_b_spatial_scalability(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int code;

	code = vs_read_bits(in, 9);
	
	if( (code >> 7) == 2 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 1;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 2);
	}else if( (code >> 7) == 3 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 1;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 2);
	}else if( (code >> 6) == 2 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 1;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 3);
	}else if( (code >> 6) == 3 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 1;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 3);
	}else if( (code >> 5) == 2 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 4);
	}else if( (code >> 5) == 3 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 4);
	}else if( (code >> 3) == 6 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 1;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 1;
		vs_erase_bits(in, 6);
	}else if( (code >> 3) == 7 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 1;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 1;
		vs_erase_bits(in, 6);
	}else if( (code >> 3) == 4 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 1;
		vs_erase_bits(in, 6);
	}else if( (code >> 3) == 5 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 1;
		vs_erase_bits(in, 6);
	}else if( (code >> 2) == 6 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 1;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 7);
	}else if( (code >> 2) == 7 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 1;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 7);
	}else if( (code >> 2) == 4 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 7);
	}else if( (code >> 2) == 5 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 1;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 7);
	}else if( (code >> 1) == 4 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 1;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 8);
	}else if( (code >> 1) == 5 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 1;
		vs_erase_bits(in, 8);
	}else if(code == 12){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 1;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 1;
		vs_erase_bits(in, 9);
	}else if(code == 14){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 4;
		vs_erase_bits(in, 9);
	}else if(code == 13){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 4;
		vs_erase_bits(in, 9);
	}else if(code == 15){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 4;
		vs_erase_bits(in, 9);
	}else{
		return 0;
	}

	return 1;
}

int read_macroblock_type_p_spatial_scalability(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int code;

	code = vs_read_bits(in, 7);

	if( (code >> 5) == 2 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 2);
	}else if( (code >> 4) == 3 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 3);
	}else if(code == 4){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 7);
	}else if( (code >> 1) == 7 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 6);
	}else if( (code >> 3) == 2 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 4);
	}else if(code == 7){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 1;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 7);
	}else if( (code >> 3) == 3 ){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 1;
		vs_erase_bits(in, 4);
	}else if( (code >> 4) == 2 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 3);
	}else if( (code >> 1) == 4 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 6);
	}else if(code == 6){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 1;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 7);
	}else if( (code >> 5) == 3 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 1;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 1;
		vs_erase_bits(in, 2);
	}else if( (code >> 1) == 5 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 1;
		vs_erase_bits(in, 6);
	}else if( (code >> 1) == 6){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 1;
		vs_erase_bits(in, 6);
	}else if(code == 5){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 4;
		vs_erase_bits(in, 7);
	}else if(code == 2){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 4;
		vs_erase_bits(in, 7);
	}else if(code == 3){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 4;
		vs_erase_bits(in, 7);
	}else{
		return 0;
	}

	return 1;
}

int read_macroblock_type_i_spatial_scalability(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int code;

	code = vs_read_bits(in, 4);

	if( (code >> 3) == 1){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 4;
		vs_erase_bits(in, 1);
	}else if( (code >> 2) == 1 ){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 1;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 4;
		vs_erase_bits(in, 2);
	}else if(code == 3){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 1;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 4);
	}else if(code == 2){
		out->macroblock_quant = 1;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 1;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 4);
	}else if(code == 1){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 0;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 4;
		vs_erase_bits(in, 4);
	}else{
		return 0;
	}

	return 1;
}

/*
QUANT 0010 0000 0000
MFWD  0001 0000 0000
MBWD  0000 1000 0000
PTTN  0000 0100 0000
INTR  0000 0010 0000
STCF  0000 0001 0000
                CLSS
*/

int read_macroblock_type_b(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int code;

	static const BASIC_VLC_ELEMENT table[] = {
		{0x220,6},{0x2C0,6},{0x340,6},{0x3C0,5},{0x3C0,5},{0x020,5},{0x020,5},
		{0x100,4},{0x100,4},{0x100,4},{0x100,4},{0x140,4},{0x140,4},{0x140,4},{0x140,4},
		{0x080,3},{0x080,3},{0x080,3},{0x080,3},{0x080,3},{0x080,3},{0x080,3},{0x080,3},
		{0x0C0,3},{0x0C0,3},{0x0C0,3},{0x0C0,3},{0x0C0,3},{0x0C0,3},{0x0C0,3},{0x0C0,3},
		{0x180,2},{0x180,2},{0x180,2},{0x180,2},{0x180,2},{0x180,2},{0x180,2},{0x180,2},
		{0x180,2},{0x180,2},{0x180,2},{0x180,2},{0x180,2},{0x180,2},{0x180,2},{0x180,2},
		{0x1C0,2},{0x1C0,2},{0x1C0,2},{0x1C0,2},{0x1C0,2},{0x1C0,2},{0x1C0,2},{0x1C0,2},
		{0x1C0,2},{0x1C0,2},{0x1C0,2},{0x1C0,2},{0x1C0,2},{0x1C0,2},{0x1C0,2},{0x1C0,2},
	};

	code = vs_read_bits(in, 6);
	if(code == 0){
		return 0;
	}
	code -= 1;

	vs_erase_bits(in, table[code].length);

	out->macroblock_quant = (table[code].value >> 9) & 1;
	out->macroblock_motion_forward = (table[code].value >> 8) & 1;
	out->macroblock_motion_backward = (table[code].value >> 7) & 1;
	out->macroblock_pattern = (table[code].value >> 6) & 1;
	out->macroblock_intra = (table[code].value >> 5) & 1;
	out->spatial_temporal_weight_code_flag = 0;
	out->spatial_temporal_weight_class = 0;

	return 1;
}

int read_macroblock_type_p(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int code;

	static const BASIC_VLC_ELEMENT table[] = {
		{0x220,6},{0x240,5},{0x240,5},{0x340,5},{0x340,5},{0x020,5},{0x020,5},
		{0x100,3},{0x100,3},{0x100,3},{0x100,3},{0x100,3},{0x100,3},{0x100,3},{0x100,3},
		{0x040,2},{0x040,2},{0x040,2},{0x040,2},{0x040,2},{0x040,2},{0x040,2},{0x040,2},
		{0x040,2},{0x040,2},{0x040,2},{0x040,2},{0x040,2},{0x040,2},{0x040,2},{0x040,2},
		{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},
		{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},
		{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},
		{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},{0x140,1},
	};
	
	code = vs_read_bits(in, 6);
	if(code == 0){
		return 0;
	}
	code -= 1;
	
	vs_erase_bits(in, table[code].length);

	out->macroblock_quant = (table[code].value >> 9) & 1;
	out->macroblock_motion_forward = (table[code].value >> 8) & 1;
	out->macroblock_motion_backward = 0;
	out->macroblock_pattern = (table[code].value >> 6) & 1;
	out->macroblock_intra = (table[code].value >> 5) & 1;
	out->spatial_temporal_weight_code_flag = 0;
	out->spatial_temporal_weight_class = 0;

	return 1;
}

int read_macroblock_type_i(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int code;

	static const BASIC_VLC_ELEMENT table[] = {
		{0x220,2},{0x020,1},{0x020,1},
	};
	
	code = vs_read_bits(in, 2);
	if(code == 0){
		return 0;
	}
	code -= 1;

	vs_erase_bits(in, table[code].length);

	out->macroblock_quant = (table[code].value >> 9) & 1;
	out->macroblock_motion_forward = 0;
	out->macroblock_motion_backward = 0;
	out->macroblock_pattern = 0;
	out->macroblock_intra = (table[code].value >> 5) & 1;
	out->spatial_temporal_weight_code_flag = 0;
	out->spatial_temporal_weight_class = 0;

	return 1;
}

int read_macroblock_type_d(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int code;

	code = vs_read_bits(in, 2);

	if(code == 3){
		out->macroblock_quant = 0;
		out->macroblock_motion_forward = 0;
		out->macroblock_motion_backward = 0;
		out->macroblock_pattern = 0;
		out->macroblock_intra = 1;
		out->spatial_temporal_weight_code_flag = 0;
		out->spatial_temporal_weight_class = 0;
		vs_erase_bits(in, 2);
		return 1;
	}

	return 0;
}

static int get_motion_code(VIDEO_STREAM *in)
{
	int r;
	int code;
	
	static const BASIC_VLC_ELEMENT table_a[] = {
		{ 2, 4}, {-2, 4},
		{ 1, 3}, { 1, 3}, {-1,3}, {-1, 3},
		{ 0, 1}, { 0, 1}, { 0,1}, { 0, 1},
		{ 0, 1}, { 0, 1}, { 0,1}, { 0, 1},
		{ 0, 0}, { 0, 0}, { 0,0}, { 0, 0},
	};
	
	static const BASIC_VLC_ELEMENT table_b[] = {
		{ 7, 8}, {-7, 8}, { 6, 8}, {-6, 8}, { 5, 8}, {-5, 8},
		{ 4, 7}, { 4, 7}, {-4, 7}, {-4, 7},
		{ 3, 5}, { 3, 5}, { 3, 5}, { 3, 5},
		{ 3, 5}, { 3, 5}, { 3, 5}, { 3, 5},
		{-3, 5}, {-3, 5}, {-3, 5}, {-3, 5},
		{-3, 5}, {-3, 5}, {-3, 5}, {-3, 5},
	};
	
	static const BASIC_VLC_ELEMENT table_c[] = {
		{16,11}, {-16,11}, {15,11}, {-15,11},
		{14,11}, {-14,11}, {13,11}, {-13,11},
		{12,11}, {-12,11}, {11,11}, {-11,11},
		{10,10}, {10,10}, {-10,10}, {-10,10},
		{ 9,10}, { 9,10}, { -9,10}, { -9,10},
		{ 8,10}, { 8,10}, { -8,10}, { -8,10},
	};

	code = vs_read_bits(in, 11);

	if(code > 255){
		code = (code >> 7) - 2;
		r = table_a[code].value;
		vs_erase_bits(in, table_a[code].length);
	}else if(code > 47){
		code = (code >> 3) - 6;
		r = table_b[code].value;
		vs_erase_bits(in, table_b[code].length);
	}else if(code > 23){
		code -= 24;
		r = table_c[code].value;
		vs_erase_bits(in, table_c[code].length);
	}else{
		r = 0;
	}

	return r;
}

static int get_dmvector(VIDEO_STREAM *in)
{
	int code;
	
	static const BASIC_VLC_ELEMENT table[] = {
		{ 0, 1}, { 0, 1}, { 1, 2}, {-1, 2}
	};

	code = vs_read_bits(in, 2);

	vs_erase_bits(in, table[code].length);

	return table[code].value;
}

static void read_motion_vector(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt, int s)
{
	int r,t;
	int half;

	if( (out->motion_vector_format == MOTION_VECTOR_FORMAT_FIELD) && (opt->picture_structure == 3) ){
		half = 2;
	}else if( opt->full_pel_vector[s] ){
		half = 3;
	}else{
		half = 0;
	}
	
	if(out->motion_vector_count == 1){
		if( (out->motion_vector_format == MOTION_VECTOR_FORMAT_FIELD) && (out->dual_prime_motion_vector != 1) ){
			out->motion_vertical_field_select[0][s] = vs_get_bits(in, 1);
		}
		for(t=0;t<2;t++){
			out->PMV[0][s][t] = get_motion_vector_predictor(in, out->PMV[0][s][t], opt->f_code[s][t], (half & (t+1)) );
			if(out->dual_prime_motion_vector == 1){
				read_dmv(in, out, opt, s, t);
			}
			out->PMV[1][s][t] = out->PMV[0][s][t];
		}
	}else{
		for(r=0;r<2;r++){
			out->motion_vertical_field_select[r][s] = vs_get_bits(in, 1);
			for(t=0;t<2;t++){
				out->PMV[r][s][t] = get_motion_vector_predictor(in, out->PMV[r][s][t], opt->f_code[s][t], (half && t));
			}
		}
	}
}

static void read_dmv(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt, int s, int t)
{
	int v;
	int dmvector;

	dmvector = get_dmvector(in);
	
	if(opt->picture_structure == 3){
		if(t){
			v = out->PMV[0][s][t] >> 1;
		}else{
			v = out->PMV[0][s][t];
		}
		if(opt->top_field_first){
			out->DMV[0][t] = ( ( v   + (v>0) )>>1 ) + dmvector - t;
			out->DMV[1][t] = ( ( v*3 + (v>0) )>>1 ) + dmvector + t;
		}else{
			out->DMV[0][t] = ( ( v*3 + (v>0) )>>1 ) + dmvector - t;
			out->DMV[1][t] = ( ( v   + (v>0) )>>1 ) + dmvector + t;
		}
	}else{
		v = out->PMV[0][s][t];
		out->DMV[0][t] = ( ( v + (v>0) )>>1) + dmvector;
		if(opt->picture_structure == 1){
			out->DMV[0][t] -= t;
		}else{
			out->DMV[0][t] += t;
		}
	}
}

static int get_motion_vector_predictor(VIDEO_STREAM *in, int prediction, int f_code, int half)
{
	int r;
	
	int f;
	int r_size;
	int high;
	int low;
	int range;

	int delta;
	
	int motion_code;
	int motion_residual;

	r_size = f_code - 1;
	f = 1 << r_size;

	high = 16 * f - 1;
	low = -16 * f;
	range = 32 * f;

	motion_code = get_motion_code(in);
	if( (!r_size) || (motion_code == 0) ){
		delta = motion_code;
	}else{
		motion_residual = vs_get_bits(in, r_size);
		delta = (abs(motion_code) - 1) * f + motion_residual + 1;
		if(motion_code < 0){
			delta = 0 - delta;
		}
	}

	if(half){
		prediction >>= 1;
	}

	r = prediction + delta;

	if(r < low){
		r += range;
	}else if(r > high){
		r -= range;
	}

	if(half){
		r <<= 1;
	}

	return r;
}

static void read_coded_block_mpeg2_420_intra(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;

	out->block_count = 6;

	for(i=0;i<4;i++){
		out->read_block[i] = read_block_mpeg2_intra_luminance;
	}
	out->read_block[4] = read_block_mpeg2_intra_cb;
	out->read_block[5] = read_block_mpeg2_intra_cr;
}

static void read_coded_block_mpeg1_420_intra(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;

	out->block_count = 6;

	for(i=0;i<4;i++){
		out->read_block[i] = read_block_mpeg1_intra_luminance;
	}
	out->read_block[4] = read_block_mpeg1_intra_cb;
	out->read_block[5] = read_block_mpeg1_intra_cr;
}

static void read_coded_block_mpeg2_422_intra(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;
	out->block_count = 8;

	for(i=0;i<4;i++){
		out->read_block[i] = read_block_mpeg2_intra_luminance;
	}
	out->read_block[4] = read_block_mpeg2_intra_cb;
	out->read_block[5] = read_block_mpeg2_intra_cr;
	out->read_block[6] = read_block_mpeg2_intra_cb;
	out->read_block[7] = read_block_mpeg2_intra_cr;
}

static void read_coded_block_mpeg2_444_intra(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;
	out->block_count = 12;

	for(i=0;i<4;i++){
		out->read_block[i] = read_block_mpeg2_intra_luminance;
	}
	for(i=4;i<12;i+=2){
		out->read_block[i] = read_block_mpeg2_intra_cb;
		out->read_block[i+1] = read_block_mpeg2_intra_cr;
	}
}

static void read_coded_block_mpeg2_420_nonintra_pattern(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;
	int coded_block_pattern;

	static const READ_BLOCK rb_y_table[2] = {
		read_block_null,
		read_block_mpeg2_nonintra_luminance,
	};

	static const READ_BLOCK rb_c_table[2] = {
		read_block_null,
		read_block_mpeg2_nonintra_chrominance,
	};

	out->block_count = 6;

	coded_block_pattern = get_coded_block_pattern(in);
	
	for(i=0;i<4;i++){
		out->read_block[i] = rb_y_table[ ((coded_block_pattern >> (out->block_count - 1 - i)) & 1) ];
	}
	for(i=4;i<6;i++){
		out->read_block[i] = rb_c_table[ ((coded_block_pattern >> (out->block_count - 1 - i)) & 1) ];
	}
}

static void read_coded_block_mpeg1_420_nonintra_pattern(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;
	int coded_block_pattern;

	static const READ_BLOCK rb_table[2] = {
		read_block_null,
		read_block_mpeg1_nonintra,
	};

	out->block_count = 6;

	coded_block_pattern = get_coded_block_pattern(in);
	
	for(i=0;i<6;i++){
		out->read_block[i] = rb_table[ ((coded_block_pattern >> (out->block_count - 1 - i)) & 1) ];
	}
}

static void read_coded_block_mpeg2_422_nonintra_pattern(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;
	int coded_block_pattern;

	static const READ_BLOCK rb_y_table[2] = {
		read_block_null,
		read_block_mpeg2_nonintra_luminance,
	};

	static const READ_BLOCK rb_c_table[2] = {
		read_block_null,
		read_block_mpeg2_nonintra_chrominance,
	};

	out->block_count = 8;

	coded_block_pattern = get_coded_block_pattern(in);
	coded_block_pattern <<= 2;
	coded_block_pattern += vs_get_bits(in, 2);
	
	for(i=0;i<4;i++){
		out->read_block[i] = rb_y_table[ ((coded_block_pattern >> (out->block_count - 1 - i)) & 1) ];
	}
	for(i=4;i<8;i++){
		out->read_block[i] = rb_c_table[ ((coded_block_pattern >> (out->block_count - 1 - i)) & 1) ];
	}
}

static void read_coded_block_mpeg2_444_nonintra_pattern(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;
	int coded_block_pattern;

	static const READ_BLOCK rb_y_table[2] = {
		read_block_null,
		read_block_mpeg2_nonintra_luminance,
	};

	static const READ_BLOCK rb_c_table[2] = {
		read_block_null,
		read_block_mpeg2_nonintra_chrominance,
	};

	out->block_count = 12;

	coded_block_pattern = get_coded_block_pattern(in);
	coded_block_pattern <<= 6;
	coded_block_pattern += vs_get_bits(in, 6);
	
	for(i=0;i<4;i++){
		out->read_block[i] = rb_y_table[ ((coded_block_pattern >> (out->block_count - 1 - i)) & 1) ];
	}
	for(i=4;i<12;i++){
		out->read_block[i] = rb_c_table[ ((coded_block_pattern >> (out->block_count - 1 - i)) & 1) ];
	}
}

static void read_coded_block_420_nonintra_nopattern(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;
	
	out->block_count = 6;
	
	for(i=0;i<6;i++){
		out->read_block[i] = read_block_null;
	}
}

static void read_coded_block_422_nonintra_nopattern(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;
	
	out->block_count = 8;
	
	for(i=0;i<8;i++){
		out->read_block[i] = read_block_null;
	}
}

static void read_coded_block_444_nonintra_nopattern(VIDEO_STREAM *in, MACROBLOCK *out)
{
	int i;
	
	out->block_count = 12;
	
	for(i=0;i<12;i++){
		out->read_block[i] = read_block_null;
	}
}

static int get_coded_block_pattern(VIDEO_STREAM *in)
{
	int r;
	int code;
	
	static const BASIC_VLC_ELEMENT table_a[] = {
		{63,6},{ 3,6},{36,6},{24,6},{62,5},{62,5},{ 2,5},{ 2,5},
		{61,5},{61,5},{ 1,5},{ 1,5},{56,5},{56,5},{52,5},{52,5},
		{44,5},{44,5},{28,5},{28,5},{40,5},{40,5},{20,5},{20,5},
		{48,5},{48,5},{12,5},{12,5},{32,4},{32,4},{32,4},{32,4},
		{16,4},{16,4},{16,4},{16,4},{ 8,4},{ 8,4},{ 8,4},{ 8,4},
		{ 4,4},{ 4,4},{ 4,4},{ 4,4},{60,3},{60,3},{60,3},{60,3},
		{60,3},{60,3},{60,3},{60,3},
	};
	
	static const BASIC_VLC_ELEMENT table_b[] = {
		{ 0,0},{ 0,9},{39,9},{27,9},{59,9},{55,9},{47,9},{31,9},
		{58,8},{58,8},{54,8},{54,8},{46,8},{46,8},{30,8},{30,8},
		{57,8},{57,8},{53,8},{53,8},{45,8},{45,8},{29,8},{29,8},
		{38,8},{38,8},{26,8},{26,8},{37,8},{37,8},{25,8},{25,8},
		{43,8},{43,8},{23,8},{23,8},{51,8},{51,8},{15,8},{15,8},
		{42,8},{42,8},{22,8},{22,8},{50,8},{50,8},{14,8},{14,8},
		{41,8},{41,8},{21,8},{21,8},{49,8},{49,8},{13,8},{13,8},
		{35,8},{35,8},{19,8},{19,8},{11,8},{11,8},{ 7,8},{ 7,8},
		{34,7},{34,7},{34,7},{34,7},{18,7},{18,7},{18,7},{18,7},
		{10,7},{10,7},{10,7},{10,7},{ 6,7},{ 6,7},{ 6,7},{ 6,7},
		{33,7},{33,7},{33,7},{33,7},{17,7},{17,7},{17,7},{17,7},
		{ 9,7},{ 9,7},{ 9,7},{ 9,7},{ 5,7},{ 5,7},{ 5,7},{ 5,7},
	};

	code = vs_read_bits(in, 9);

	if(code > 95){
		code = (code >> 3) - 12;
		r = table_a[code].value;
		vs_erase_bits(in, table_a[code].length);
	}else{
		r = table_b[code].value;
		vs_erase_bits(in, table_b[code].length);
	}

	return r;
}

typedef struct {
	int weight_top;
	int weight_bottom;
	int class;
	int integer_weight;
} SPATIAL_TEMPORAL_WEIGHT_TABLE_ELEMENT;

static void read_spatial_temporal_weight(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt)
{
	int code;
	
	static const SPATIAL_TEMPORAL_WEIGHT_TABLE_ELEMENT table[4][4] = {
		{
			{1, 1, 1, 0},
			{1, 1, 1, 0},
			{1, 1, 1, 0},
			{1, 1, 1, 0},
		}, /* table_index_0 */

		{
			{0, 2, 3, 1},
			{0, 1, 1, 0},
			{1, 2, 3, 0},
			{1, 1, 1, 0},
		}, /* table_index_1 */

		{
			{2, 0, 2, 1},
			{1, 0, 1, 0},
			{2, 1, 2, 0},
			{1, 1, 1, 0},
		}, /* table_index_2 */

		{
			{2, 0, 2, 1},
			{2, 1, 2, 0},
			{1, 2, 3, 0},
			{1, 1, 1, 0},
		}, /* table_index_4 */
	};
	
	if(opt->spatial_temporal_weight_code_table_index){
		code = vs_get_bits(in, 2);
		out->spatial_temporal_weight[0] = table[opt->spatial_temporal_weight_code_table_index][code].weight_top;
		out->spatial_temporal_weight[1] = table[opt->spatial_temporal_weight_code_table_index][code].weight_bottom;
		out->spatial_temporal_weight_class = table[opt->spatial_temporal_weight_code_table_index][code].class;
		out->spatial_temporal_integer_weight = table[opt->spatial_temporal_weight_code_table_index][code].integer_weight;
	}else{
		out->spatial_temporal_weight[0] = 1;
		out->spatial_temporal_weight[1] = 1;
		out->spatial_temporal_weight_class = 1;
		out->spatial_temporal_integer_weight = 0;
	}
}