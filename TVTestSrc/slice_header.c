/*******************************************************************
                         slice layer interface
 *******************************************************************/
#include <stdio.h>

#define SLICE_HEADER_C
#include "slice_header.h"

int read_slice_header(VIDEO_STREAM *in, SLICE_HEADER *out, READ_SLICE_HEADER_OPTION *opt);
int slice_header_to_read_block_option(SLICE_HEADER *in, READ_BLOCK_OPTION *out);

int read_slice_header(VIDEO_STREAM *in, SLICE_HEADER *out, READ_SLICE_HEADER_OPTION *opt)
{
	int code;

	code = vs_read_bits(in, 32);
	code -= 0x100;
	if(code > 0 && code < 0xB0){
		out->slice_vertical_position = code;
		vs_erase_bits(in, 32);
	}else{
		return 0;
	}

	if(opt->v_size > 2800){
		out->slice_vertical_position += vs_get_bits(in, 3) << 7;
	}
	
	if(opt->scalable_mode == 0){
		out->priority_break_point = vs_get_bits(in, 7);
	}

	out->quantizer_scale_code = vs_get_bits(in, 5);

	out->intra_slice_flag = vs_get_bits(in, 1);
	if(out->intra_slice_flag){
		
		out->intra_slice = vs_get_bits(in, 1);
		
		out->slice_picture_id_enable = vs_get_bits(in, 1);
		out->slice_picture_id = vs_get_bits(in, 6);

		while(vs_get_bits(in, 1)){
			vs_get_bits(in, 8);
		}
	}else{
		out->intra_slice = 0;
	}

	return 1;
}

int slice_header_to_read_block_option(SLICE_HEADER *in, READ_BLOCK_OPTION *out)
{
	out->qw[0] = out->qw_table[in->quantizer_scale_code][0];
	out->qw[1] = out->qw_table[in->quantizer_scale_code][1];
	out->qw[2] = out->qw_table[in->quantizer_scale_code][2];
	out->qw[3] = out->qw_table[in->quantizer_scale_code][3];
	
	reset_dc_dct_predictor(out);
	
	return 1;
}
