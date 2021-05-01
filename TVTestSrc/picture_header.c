/*******************************************************************
            MPEG Video picture header reading routine
 *******************************************************************/
#include <stdlib.h>

#include "scan.h"

#define PICTURE_HEADER_C
#include "picture_header.h"

int read_picture_header(VIDEO_STREAM *in, PICTURE_HEADER *out, READ_PICTURE_HEADER_OPTION *opt);
int picture_header_to_output_parameter(PICTURE_HEADER *in, OUTPUT_PARAMETER *out);
int picture_header_to_read_macroblock_option(PICTURE_HEADER *in, READ_MACROBLOCK_OPTION *out);
int picture_header_to_read_block_option(PICTURE_HEADER *in, READ_BLOCK_OPTION *out);
int picture_header_to_mc_parameter(PICTURE_HEADER *in, MC_PARAMETER *out);

int check_field_picture(PICTURE_HEADER *in);

static int read_picture_coding_extension(VIDEO_STREAM *in, PICTURE_CODING_EXTENSION *out);
static int read_quant_matrix_extension(VIDEO_STREAM *in, QUANT_MATRIX_EXTENSION *out);
static int read_picture_display_extension(VIDEO_STREAM *in, PICTURE_DISPLAY_EXTENSION *out, int progressive_sequence, int picture_structure, int repeat_first_field);
static int read_picture_spatial_scalable_extension(VIDEO_STREAM *in, PICTURE_SPATIAL_SCALABLE_EXTENSION *out);
static int read_temporal_scalable_extension(VIDEO_STREAM *in, PICTURE_TEMPORAL_SCALABLE_EXTENSION *out);
static int read_copyright_extension(VIDEO_STREAM *in, COPYRIGHT_EXTENSION *out);


int read_picture_header(VIDEO_STREAM *in, PICTURE_HEADER *out, READ_PICTURE_HEADER_OPTION *opt)
{
	int code;
	
	out->temporal_reference = vs_get_bits(in, 10);
	out->picture_coding_type = vs_get_bits(in, 3);

	out->vbv_delay = vs_get_bits(in, 16);
	if(out->picture_coding_type == 2 || out->picture_coding_type == 3){
		out->full_pel_forward_vector = vs_get_bits(in, 1);
		out->forward_f_code = vs_get_bits(in, 3);
	}else{
		out->full_pel_forward_vector = 0;
		out->forward_f_code = 0;	
	}
	
	if(out->picture_coding_type == 3){
		out->full_pel_backward_vector = vs_get_bits(in, 1);
		out->backward_f_code = vs_get_bits(in, 3);
	}else{
		out->full_pel_backward_vector = 0;
		out->backward_f_code = 0;
	}
	
	while((code = vs_get_bits(in, 1)) == 1){
		vs_get_bits(in, 8);  /* read skip EBP & EIP */
	}

	out->has_picture_coding_extension = 0;
	out->has_quant_matrix_extension = 0;
	out->has_picture_display_extension = 0;
	out->has_picture_spatial_scalable_extension = 0;
	out->has_temporal_scalable_extension = 0;
	out->has_copyright_extension = 0;

	while(vs_next_start_code(in)){
		code = vs_read_bits(in, 32);
		if(code == 0x1B2){ /* user data */
			vs_erase_bits(in, 32);
			continue;
		}else if(code != 0x1B5){
			break;
		}
		vs_erase_bits(in, 32);
		
		code = vs_get_bits(in, 4);
		switch(code){
		case 3:
			out->has_quant_matrix_extension = 1;
			read_quant_matrix_extension(in, &(out->qm));
			break;
		case 4:
			out->has_copyright_extension = 1;
			read_copyright_extension(in, &(out->c));
			break;
		case 7:
			out->has_picture_display_extension = 1;
			read_picture_display_extension(in, &(out->pd), opt->progressive_sequence, out->pc.picture_structure, out->pc.repeat_first_field);
			break;
		case 8:
			out->has_picture_coding_extension = 1;
			read_picture_coding_extension(in, &(out->pc));
			break;
		case 9:
			out->has_picture_spatial_scalable_extension = 1;
			read_picture_spatial_scalable_extension(in, &(out->pss));
			break;
		case 10:
			out->has_temporal_scalable_extension = 1;
			read_temporal_scalable_extension(in, &(out->pts));
			break;
		default:
			break;
		}
	}

	return 1;
}

int picture_header_to_output_parameter(PICTURE_HEADER *in, OUTPUT_PARAMETER *out)
{
	out->picture_coding_type = in->picture_coding_type;

	if(in->has_picture_coding_extension){
		if (in->pc.picture_structure == 3) {
			out->repeat_first_field = in->pc.repeat_first_field;
			out->top_field_first = in->pc.top_field_first;
			out->progressive_frame = in->pc.progressive_frame;
		} else {
			out->repeat_first_field = 0;
			out->progressive_frame = 0;
			if (in->pc.picture_structure == 1) {
				out->top_field_first = 1;
			} else {
				out->top_field_first = 0;
			}
		}
	}else{
		out->repeat_first_field = 0;
		out->top_field_first = 0;
		out->progressive_frame = 1;
	}

	return 1;
}

int picture_header_to_read_macroblock_option(PICTURE_HEADER *in, READ_MACROBLOCK_OPTION *out)
{
	int i,j;
	
	out->picture_coding_type = in->picture_coding_type;

	if(in->has_picture_coding_extension){
		out->picture_structure = in->pc.picture_structure;
		out->top_field_first = in->pc.top_field_first;
		out->frame_predictive_frame_dct = in->pc.frame_predictive_frame_dct;
		out->concealment_motion_vectors = in->pc.concealment_motion_vectors;
		
		for(i=0;i<2;i++){
			for(j=0;j<2;j++){
				out->f_code[i][j] = in->pc.f_code[i][j];
			}
			out->full_pel_vector[i] = 0;
		}
	}else{
		out->picture_structure = 3;
		out->top_field_first = 0;
		out->frame_predictive_frame_dct = 1;
		out->concealment_motion_vectors = 0;

		out->f_code[0][0] = in->forward_f_code;
		out->f_code[0][1] = in->forward_f_code;
		out->f_code[1][0] = in->backward_f_code;
		out->f_code[1][1] = in->backward_f_code;

		out->full_pel_vector[0] = in->full_pel_forward_vector;
		out->full_pel_vector[1] = in->full_pel_backward_vector;
	}

	switch(out->scalable_mode){
	case 1:
		if(in->has_picture_spatial_scalable_extension){
			switch(in->picture_coding_type){
			case 1:
				out->read_macroblock_type = read_macroblock_type_i_spatial_scalability;
				break;
			case 2:
				out->read_macroblock_type = read_macroblock_type_p_spatial_scalability;
				break;
			case 3:
				out->read_macroblock_type = read_macroblock_type_b_spatial_scalability;
				break;
			}
		}else{	
			switch(in->picture_coding_type){
			case 1:
				out->read_macroblock_type = read_macroblock_type_i;
				break;
			case 2:
				out->read_macroblock_type = read_macroblock_type_p;
				break;
			case 3:
				out->read_macroblock_type = read_macroblock_type_b;
				break;
			}
		}
		break;
	case 2:
		out->read_macroblock_type = read_macroblock_type_snr_scalability;
		out->spatial_temporal_weight_code_table_index = 0;
		break;
	default:
		switch(in->picture_coding_type){
		case 1:
			out->read_macroblock_type = read_macroblock_type_i;
			break;
		case 2:
			out->read_macroblock_type = read_macroblock_type_p;
			break;
		case 3:
			out->read_macroblock_type = read_macroblock_type_b;
			break;
		}
	}

	if(in->has_picture_spatial_scalable_extension){
		out->spatial_temporal_weight_code_table_index = in->pss.spatial_temporal_weight_code_table_index;
	}else{
		out->spatial_temporal_weight_code_table_index = 0;
	}

	return 1;
}

static const int quantizer_scale[2][32] = {
	{
		 0,  2,  4,  6,  8, 10, 12, 14,
		16, 18, 20, 22, 24, 26, 28, 30,
		32, 34, 36, 38, 40, 42, 44, 46,
		48, 50, 52, 54, 56, 58, 60, 62,
	}, {
		 0,  1,  2,  3,  4,  5,  6,  7,
		 8, 10, 12, 14, 16, 18, 20, 22,
		24, 28, 32, 36, 40, 44, 48, 52,
		56, 64, 72, 80, 88, 96,104,112,
	},
};
	
int picture_header_to_read_block_option(PICTURE_HEADER *in, READ_BLOCK_OPTION *out)
{
	int i, j;
	
	if(in->has_picture_coding_extension){
		out->picture_structure = in->pc.picture_structure;
		out->intra_dc_precision = in->pc.intra_dc_precision;
		out->intra_vlc_format = in->pc.intra_vlc_format;
		out->alternate_scan = in->pc.alternate_scan;
		out->q_scale_type = in->pc.q_scale_type;
	}else{
		out->picture_structure = 3;
		out->intra_dc_precision = 0;
		out->intra_vlc_format = 0;
		out->alternate_scan = 0;
		out->q_scale_type = 0;
	}

	if(in->has_quant_matrix_extension){
		for(i=0;i<4;i++){
			if(in->qm.load_quantizer_matrix[i]){
				for(j=0;j<64;j++){
					out->quantizer_weight[i][scan_table[0][j]] = in->qm.quantizer_matrix[i][j];
				}
			}else if(i>1){
				memcpy(out->quantizer_weight[i], out->quantizer_weight[i-2], 64*sizeof(short));
			}
		}
	}

	for(i=0;i<32;i++){
		out->setup_qw((unsigned short *)out->qw_table[i], (unsigned short *)out->quantizer_weight, quantizer_scale[out->q_scale_type][i]);
	}
	
	return 1;
}

int check_field_picture(PICTURE_HEADER *in)
{
	if (in->has_picture_coding_extension == 0) {
		return 0;
	}
	if (in->pc.picture_structure == 3) {
		return 0;
	}
	return 1;
}

static int read_picture_coding_extension(VIDEO_STREAM *in, PICTURE_CODING_EXTENSION *out)
{
	out->f_code[0][0] = vs_get_bits(in, 4);
	out->f_code[0][1] = vs_get_bits(in, 4); 
	out->f_code[1][0] = vs_get_bits(in, 4);
	out->f_code[1][1] = vs_get_bits(in, 4);
	out->intra_dc_precision = vs_get_bits(in, 2);
	out->picture_structure = vs_get_bits(in, 2);
	out->top_field_first = vs_get_bits(in, 1);
	out->frame_predictive_frame_dct = vs_get_bits(in, 1);
	out->concealment_motion_vectors = vs_get_bits(in, 1);
	out->q_scale_type = vs_get_bits(in, 1);
	out->intra_vlc_format = vs_get_bits(in, 1);
	out->alternate_scan = vs_get_bits(in, 1);
	out->repeat_first_field = vs_get_bits(in, 1);
	out->chroma_420_type = vs_get_bits(in, 1);
	out->progressive_frame = vs_get_bits(in, 1);
	out->composit_display_flag = vs_get_bits(in, 1);
	if(out->composit_display_flag){
		out->v_axis = vs_get_bits(in, 1);
		out->field_sequence = vs_get_bits(in, 3);
		out->sub_carrier = vs_get_bits(in, 1);
		out->burst_amplitude = vs_get_bits(in, 7);
		out->sub_carrier_phase = vs_get_bits(in, 8);
	}

	return 1;
}

int picture_header_to_mc_parameter(PICTURE_HEADER *in, MC_PARAMETER *out)
{
	out->picture_coding_type = in->picture_coding_type;
	
	if(in->has_picture_coding_extension){
		if(in->pc.picture_structure != 3){
			out->first_field = !out->first_field;
		}
		out->picture_structure = in->pc.picture_structure;
		out->top_field_first = in->pc.top_field_first;
	}else{
		out->first_field = 0;
		out->picture_structure = 3;
		out->top_field_first = 0;
	}

	return 1;
}

static int read_quant_matrix_extension(VIDEO_STREAM *in, QUANT_MATRIX_EXTENSION *out)
{
	int i,j;

	for(i=0;i<4;i++){
		out->load_quantizer_matrix[i] = vs_get_bits(in, 1);
		if(out->load_quantizer_matrix[i]){
			for(j=0;j<64;j++){
				out->quantizer_matrix[i][j] = (unsigned char)vs_get_bits(in,8);
			}
		}
	}

	return 1;
}

static int read_picture_display_extension(VIDEO_STREAM *in, PICTURE_DISPLAY_EXTENSION *out, int progressive_sequence, int picture_structure, int repeat_first_field)
{
	int i;
	if(progressive_sequence || picture_structure == 1 || picture_structure == 2){
		out->number_of_frame_center_offset = 1;
	}else{
		if(repeat_first_field){
			out->number_of_frame_center_offset = 3;
		}else{
			out->number_of_frame_center_offset = 2;
		}
	}
	
	for(i=0;i<out->number_of_frame_center_offset;i++){
		out->frame_center_horizontal_offset[i] = vs_get_bits(in, 16);
		vs_get_bits(in, 1);
		out->frame_center_vertical_offset[i] = vs_get_bits(in, 16);
		vs_get_bits(in, 1);
	}
	return 1;
}

static int read_picture_spatial_scalable_extension(VIDEO_STREAM *in, PICTURE_SPATIAL_SCALABLE_EXTENSION *out)
{
	out->low_layer_temporal_reference = vs_get_bits(in, 10);
	out->lower_layer_horizontal_offset = vs_get_bits(in, 15);
	out->lower_layer_vertical_offset = vs_get_bits(in, 15);
	out->spatial_temporal_weight_code_table_index = vs_get_bits(in, 2);
	out->lower_layer_progressive_frame = vs_get_bits(in, 1);
	out->lower_layer_deinterlaced_field_select = vs_get_bits(in, 1);

	return 1;
}

static int read_temporal_scalable_extension(VIDEO_STREAM *in, PICTURE_TEMPORAL_SCALABLE_EXTENSION *out)
{
	out->reference_select_code = vs_get_bits(in, 2);
	out->forward_temporal_reference = vs_get_bits(in, 10);
	out->backward_temporal_reference = vs_get_bits(in, 10);

	return 1;
}

static int read_copyright_extension(VIDEO_STREAM *in, COPYRIGHT_EXTENSION *out)
{
	out->copyright_flag = vs_get_bits(in, 1);
	out->copyright_identifier = vs_get_bits(in, 8);
	out->original_or_copy = vs_get_bits(in, 1);
  
	out->reserved_data = vs_get_bits(in, 7);

	out->copyright_number_1 = vs_get_bits(in, 20);
	out->copyright_number_2 = vs_get_bits(in, 22);
	out->copyright_number_3 = vs_get_bits(in, 22);

	return 1;
}
