/*******************************************************************
                     macroblock layer interface
 *******************************************************************/
#ifndef MACROBLOCK_H
#define MACROBLOCK_H

#include "video_stream.h"
#include "block.h"
#include "mc.h"

typedef struct {
	int macroblock_quant;
	int macroblock_motion_forward;
	int macroblock_motion_backward;
	int macroblock_pattern;
	int macroblock_intra;
	int spatial_temporal_weight_code_flag;
	int spatial_temporal_weight[2]; /* 0: top, 1: bottom */ 
	int spatial_temporal_weight_class;
	int spatial_temporal_integer_weight;
	int prediction_type;
	int motion_vector_count;
	int motion_vector_format;
	int dual_prime_motion_vector;
	int motion_vertical_field_select[2][2];
	int PMV[2][2][2];
	int DMV[2][2];
	int dct_type;
	int quantizer_scale_code;
	int block_count;
	READ_BLOCK read_block[12];
} MACROBLOCK;

typedef struct {
	/* mpeg1 flag */
	int mpeg1;
		
	/* in sequence_header ->
	        sequence_extension */
	int chroma_format;

	/* in sequence_header -> 
	        sequence_scalable_extension */
	int scalable_mode;

	/* in picture_header */
	int picture_coding_type;

	/* in picture_header ->
	        picture_coding_extension */
	int picture_structure;
	int top_field_first;
	int frame_predictive_frame_dct;
	int concealment_motion_vectors;
	int f_code[2][2];
	int full_pel_vector[2];

	/* in picture_header ->
	        picture_spatial_scalable_extension */
	int spatial_temporal_weight_code_table_index;

	int (* read_macroblock_type)(VIDEO_STREAM *in, MACROBLOCK *out);

} READ_MACROBLOCK_OPTION;

#ifdef __cplusplus
extern "C" {
#endif

extern int read_macroblock(VIDEO_STREAM *in, MACROBLOCK *out, READ_MACROBLOCK_OPTION *opt);
extern int get_macroblock_address_increment(VIDEO_STREAM *in);
extern int macroblock_to_read_block_option(MACROBLOCK *in, READ_BLOCK_OPTION *out);
extern int macroblock_to_mc_parameter(MACROBLOCK *in, MC_PARAMETER *out);
extern int macroblock_to_error_concealment_mc_parameter(MACROBLOCK *in, MC_PARAMETER *out);
extern int reset_motion_vector_predictor(MACROBLOCK *p);
extern int read_macroblock_type_snr_scalability(VIDEO_STREAM *in, MACROBLOCK *out);
extern int read_macroblock_type_b_spatial_scalability(VIDEO_STREAM *in, MACROBLOCK *out);
extern int read_macroblock_type_p_spatial_scalability(VIDEO_STREAM *in, MACROBLOCK *out);
extern int read_macroblock_type_i_spatial_scalability(VIDEO_STREAM *in, MACROBLOCK *out);
extern int read_macroblock_type_b(VIDEO_STREAM *in, MACROBLOCK *out);
extern int read_macroblock_type_p(VIDEO_STREAM *in, MACROBLOCK *out);
extern int read_macroblock_type_i(VIDEO_STREAM *in, MACROBLOCK *out);
extern int read_macroblock_type_d(VIDEO_STREAM *in, MACROBLOCK *out);

#ifdef __cplusplus
}
#endif
	
#endif

