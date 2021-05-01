/*******************************************************************
                MPEG Video picture header interfaces
 *******************************************************************/
#ifndef PICTURE_HEADER_H
#define PICTURE_HEADER_H

#include "video_stream.h"
#include "out_buffer.h"
#include "macroblock.h"
#include "block.h"
#include "mc.h"

typedef struct {
	int f_code[2][2];
	int intra_dc_precision;
	int picture_structure;
	int top_field_first;
	int frame_predictive_frame_dct;
	int concealment_motion_vectors;
	int q_scale_type;
	int intra_vlc_format;
	int alternate_scan;
	int repeat_first_field;
	int chroma_420_type;
	int progressive_frame;
	int composit_display_flag;
	int v_axis;
	int field_sequence;
	int sub_carrier;
	int burst_amplitude;
	int sub_carrier_phase;
} PICTURE_CODING_EXTENSION;

typedef struct {
	int load_quantizer_matrix[4];
	unsigned short quantizer_matrix[4][64];
} QUANT_MATRIX_EXTENSION;

typedef struct {
	int number_of_frame_center_offset;
	int frame_center_horizontal_offset[3];
	int frame_center_vertical_offset[3];
} PICTURE_DISPLAY_EXTENSION;

typedef struct {
	int low_layer_temporal_reference;
	int lower_layer_horizontal_offset;
	int lower_layer_vertical_offset;
	int spatial_temporal_weight_code_table_index;
	int lower_layer_progressive_frame;
	int lower_layer_deinterlaced_field_select;
} PICTURE_SPATIAL_SCALABLE_EXTENSION;

typedef struct {
	int reference_select_code;
	int forward_temporal_reference;
	int backward_temporal_reference;
}PICTURE_TEMPORAL_SCALABLE_EXTENSION;

typedef struct {
	int copyright_flag;
	int copyright_identifier;
	int original_or_copy;
	int reserved_data;
	int copyright_number_1;
	int copyright_number_2;
	int copyright_number_3;
}COPYRIGHT_EXTENSION;

typedef struct {
	int temporal_reference;
	int picture_coding_type;
	int vbv_delay;
	int full_pel_forward_vector;
	int forward_f_code;
	int full_pel_backward_vector;
	int backward_f_code;

	int has_picture_coding_extension;
	PICTURE_CODING_EXTENSION pc;

	int has_quant_matrix_extension;
	QUANT_MATRIX_EXTENSION qm;

	int has_picture_display_extension;
	PICTURE_DISPLAY_EXTENSION pd;

	int has_picture_spatial_scalable_extension;
	PICTURE_SPATIAL_SCALABLE_EXTENSION pss;

	int has_temporal_scalable_extension;
	PICTURE_TEMPORAL_SCALABLE_EXTENSION pts;

	int has_copyright_extension;
	COPYRIGHT_EXTENSION c;
	
} PICTURE_HEADER;

typedef struct {
	/* in sequence_header ->
	        sequence_extension */
	int progressive_sequence;
} READ_PICTURE_HEADER_OPTION;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PICTURE_HEADER_C
extern int read_picture_header(VIDEO_STREAM *in, PICTURE_HEADER *out, READ_PICTURE_HEADER_OPTION *opt);
extern int picture_header_to_output_parameter(PICTURE_HEADER *in, OUTPUT_PARAMETER *out);
extern int picture_header_to_read_macroblock_option(PICTURE_HEADER *in, READ_MACROBLOCK_OPTION *out);
extern int picture_header_to_read_block_option(PICTURE_HEADER *in, READ_BLOCK_OPTION *out);
extern int picture_header_to_mc_parameter(PICTURE_HEADER *in, MC_PARAMETER *out);

extern int check_field_picture(PICTURE_HEADER *in);
#endif

#ifdef __cplusplus
}
#endif

#endif
