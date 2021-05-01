/*******************************************************************
   MPEG Video sequance header reading routine
 *******************************************************************/

#include "scan.h"

#define SEQUENCE_HEADER_C
#include "sequence_header.h"

int read_sequence_header(VIDEO_STREAM *in, SEQUENCE_HEADER *out);
int sequence_header_to_bgr_conversion_parameter(SEQUENCE_HEADER *in, BGR_CONVERSION_PARAMETER *out, M2V_CONFIG *prm);
int sequence_header_to_yuy2_conversion_parameter(SEQUENCE_HEADER *in, YUY2_CONVERSION_PARAMETER *out, M2V_CONFIG *prm);
int sequence_header_to_read_picture_header_option(SEQUENCE_HEADER *in, READ_PICTURE_HEADER_OPTION *out);
int sequence_header_to_read_slice_header_option(SEQUENCE_HEADER *in, READ_SLICE_HEADER_OPTION *out);
int sequence_header_to_read_macroblock_option(SEQUENCE_HEADER *in, READ_MACROBLOCK_OPTION *out);
int sequence_header_to_read_block_option(SEQUENCE_HEADER *in, READ_BLOCK_OPTION *out);
int sequence_header_to_mc_parameter(SEQUENCE_HEADER *in, MC_PARAMETER *out);

int read_sequence_header(VIDEO_STREAM *in, SEQUENCE_HEADER *out)
{
	int i;
	int code;
	
	out->orig_h_size = vs_get_bits(in, 12);
	out->orig_v_size = vs_get_bits(in, 12);
	out->aspect_ratio = vs_get_bits(in, 4);
	out->picture_rate = vs_get_bits(in, 4);
	out->bit_rate = vs_get_bits(in, 18);
	vs_get_bits(in, 1); /* marker bit */
	out->vbv_buffer_size = vs_get_bits(in, 10);
	out->mpeg1 = vs_get_bits(in, 1);
	out->has_intra_quantizer_matrix = vs_get_bits(in, 1);
	if(out->has_intra_quantizer_matrix){
		for(i=0;i<64;i++){
			out->iqm[i] = (unsigned char)vs_get_bits(in, 8);
		}
	}
	out->has_nonintra_quantizer_matrix = vs_get_bits(in, 1);
	if(out->has_nonintra_quantizer_matrix){
		for(i=0;i<64;i++){
			out->nqm[i] = (unsigned char)vs_get_bits(in, 8);
		}
	}

	out->has_sequence_extension = 0;
	out->has_sequence_display_extension = 0;
	out->has_sequence_scalable_extension = 0;

	while(vs_next_start_code(in)){
		code = vs_read_bits(in, 32);
		if(code != 0x1b5){
			break;
		}
		vs_erase_bits(in, 32);

		code = vs_get_bits(in, 4);
		switch(code){
		case 1:
			out->has_sequence_extension = 1;
			out->se.profile_and_level = vs_get_bits(in, 8);
			out->se.progressive = vs_get_bits(in, 1);
			out->se.chroma_format = vs_get_bits(in, 2);
			out->orig_h_size += vs_get_bits(in, 2) << 12;
			out->orig_v_size += vs_get_bits(in, 2) << 12;
			out->bit_rate += vs_get_bits(in, 12) << 18;
			vs_erase_bits(in, 1); /* marker bits */
			out->vbv_buffer_size += vs_get_bits(in, 8) << 10;
			out->se.low_delay = vs_get_bits(in, 1);
			out->se.frame_rate_ext_n = vs_get_bits(in, 2);			
			out->se.frame_rate_ext_d = vs_get_bits(in, 2);
			break;
		case 2:
			out->has_sequence_display_extension = 1;
			out->sd.video_format = vs_get_bits(in, 3);
			out->sd.has_color_description = vs_get_bits(in, 1);
			if(out->sd.has_color_description){
				out->sd.color_primaries = vs_get_bits(in, 8);
				out->sd.transfer_characteristics = vs_get_bits(in, 8);
				out->sd.matrix_coefficients = vs_get_bits(in, 8);
			}
			out->sd.display_h_size = vs_get_bits(in, 14);
			vs_erase_bits(in, 1);
			out->sd.display_v_size = vs_get_bits(in, 14);
			break;
		case 5:
			out->has_sequence_scalable_extension = 1;
			out->ss.scalable_mode = vs_get_bits(in, 2);
			out->ss.layer = vs_get_bits(in, 4);
			if(out->ss.scalable_mode == 1){
				out->ss.lower_layer_prediction_h_size = vs_get_bits(in, 14);
				out->ss.lower_layer_prediction_v_size = vs_get_bits(in, 14);
				out->ss.h_subsampling_facter_m = vs_get_bits(in, 5);
				out->ss.h_subsampling_facter_n = vs_get_bits(in, 5);
				out->ss.v_subsampling_facter_m = vs_get_bits(in, 5);	
				out->ss.v_subsampling_facter_n = vs_get_bits(in, 5);
			}else if(out->ss.scalable_mode == 3){
				out->ss.picture_mux_enable = vs_get_bits(in, 1);
				out->ss.mux_to_progressive_sequence = vs_get_bits(in, 1);
				out->ss.pixture_mux_order = vs_get_bits(in, 3);
				out->ss.pixture_mux_facter = vs_get_bits(in, 3);
			}
			break;
		default:
			break;
		}
	}

	out->h_size = out->orig_h_size + 15;
	out->h_size >>= 4;
	out->h_size <<= 4;

	if(out->has_sequence_extension && (out->se.progressive == 0)){
		out->v_size = out->orig_v_size + 31;
		out->v_size >>= 5;
		out->v_size <<= 5;
	}else{
		out->v_size = out->orig_v_size + 15;
		out->v_size >>= 4;
		out->v_size <<= 4;
	}

	return 1;
}

int sequence_header_to_bgr_conversion_parameter(SEQUENCE_HEADER *in, BGR_CONVERSION_PARAMETER *out, M2V_CONFIG *prm)
{
	int c,n;

	static const int rate[16] = {
		 0, 24, 24, 25, 30, 30, 50, 60,
		60,  0,  0,  0,  0,  0,  0,  0,
	};
	
	static const int table[8][2][5] = {
		{
			{  /*   RV      GU      GV      BU  */
				 91881, -22553, -46802, 116130,
			}, /* straight conversion */
	
			{  /*   RV      GU      GV      BU  */
				104597, -25675, -53279, 132201,
			}, /* re-map conversion */

		}, /* no sequence_display_extension - ITU-R BT.601 YCbCr */

		{
			{  /*   RV      GU      GV      BU  */
				103206, -12276, -30679, 121609,
			}, /* straight conversion */

			{  /*   RV      GU      GV      BU  */
				117489, -13975, -34925, 138438,
			}, /* re-map conversion */
		
		}, /* ITU-R Rec. 709 */

		{
			{  /*   RV      GU      GV      BU  */
				 91881, -22553, -46802, 116130,
			}, /* straight conversion */
	
			{  /*   RV      GU      GV      BU  */
				104597, -25675, -53279, 132201,
			}, /* re-map conversion */
		
		}, /* unspecified video */

		{
			{  /*   RV      GU      GV      BU  */
				 91881, -22553, -46802, 116130,
			}, /* straight conversion */
	
			{  /*   RV      GU      GV      BU  */
				104597, -25675, -53279, 132201,
			}, /* re-map conversion */
		
		}, /* reserved */

		{
			{  /*   RV      GU      GV      BU  */
				 91750, -21749, -46653, 116654,
			}, /* straight conversion */
	
			{  /*   RV      GU      GV      BU  */
				104448, -24759, -53109, 132798,
			}, /* re-map conversion */

		}, /* FCC */

		{
			{  /*   RV      GU      GV      BU  */     
				 91881, -22553, -46801, 116129,
			}, /* straight conversion */
	
			{  /*   RV      GU      GV      BU  */
				104597, -25675, -53279, 132201,
			}, /* re-map conversion */
		
		}, /* ITU-R Rec. 470-2 System B, G */

		{
			{  /*   RV      GU      GV      BU  */
				 91881, -22553, -46802, 116130,
			}, /* straight conversion */
	
			{  /*   RV      GU      GV      BU  */
				104597, -25675, -53279, 132201,
			}, /* re-map conversion */

		}, /* SMPTE 170M */

		{
			{  /*   RV      GU      GV      BU  */
				103285, -14852, -31236, 119669,
			}, /* straight conversion */

			{  /*   RV      GU      GV      BU  */
				117579, -16907, -35559, 136230,
			}, /* re-map conversion */

		}, /* SMPTE 240 */
	};

	static const int y_offset[2] = {
		0, 16,
	};

	static const int y_gain[2] = {
		65536, 76309,
	};

	if(in->has_sequence_display_extension && in->sd.has_color_description){
		c = in->sd.matrix_coefficients;
	}else{
		if(in->has_sequence_extension){ /* MPEG-2 */
			if(prm->color_matrix){
				c = prm->color_matrix;
			}else{ /* auto select */
				n = rate[in->picture_rate];
				n *= (in->se.frame_rate_ext_n + 1);
				n /= (in->se.frame_rate_ext_d + 1);
				n *= in->orig_h_size * in->orig_v_size;
				if(n < 12000000){ /* ITU-R BT.601 */
					c = 0;
				}else{ /* ITU-R BT.709 */
					c = 1;
				}
			}
		}else{ /* MPEG-1 */
			c = 0;
		}
	}
	
	out->yo = y_offset[prm->bt601];
	out->yg = y_gain[prm->bt601];

	out->rv = table[c][prm->bt601][0];

	out->gu = table[c][prm->bt601][1];
	out->gv = table[c][prm->bt601][2];

	out->bu = table[c][prm->bt601][3];

	out->prm.width = in->orig_h_size;
	out->prm.height = in->orig_v_size;

	out->prm.in_step = in->h_size;

	if(in->has_sequence_extension){ /* MPEG-2 */
		if(in->se.chroma_format == 1){
			out->prm.c_offset = out->prm.width/2;
		}else{
			out->prm.c_offset = 0;
		}
	}else{ /* MPEG-1 */
		out->prm.c_offset = out->prm.width/2;
	}
	
	return 1;
}

int sequence_header_to_yuy2_conversion_parameter(SEQUENCE_HEADER *in, YUY2_CONVERSION_PARAMETER *out, M2V_CONFIG *prm)
{
	int c,n;

	static const int rate[16] = {
		 0, 24, 24, 25, 30, 30, 50, 60,
		60,  0,  0,  0,  0,  0,  0,  0,
	};

	static const YUY2_CONVERSION_PARAMETER table[] = {
		{ 6657,12880,64871,-7252,-4748,64448 }, // BT.709 to BT.601
		{ 532,48,65532,-27,-379,65408 },        // FCC to BT.601
		{ 4924,12547,64754,-7081,-3512,64720 }, // SMPTE 240 to BT.601

		{ -7746,-13939,66758,7512,4918,67196 }, // BT.601 to BT.709
		{ -7133,-13860,66710,7469,4529,67063 }, // FCC to BT.709
		{ -1982,-382,65559,206,1259,65828 },    // SMPTE 240 to BT.709

		{ -532,-49,65540,27,380,65664 },        // BT.601 to FCC
		{ 6134,12861,64873,-7225,-4381,64532 }, // BT.709 to FCC
		{ 4401,12556,64757,-7054,-3144,64806 }, // SMPTE 240 to FCC

		{ -5707,-13329,66723,7300,3621,66758 }, // BT.601 to SMPTE 240
		{ 1974,374,65517,-205,-1253,65249 },    // BT.709 to SMPTE 240
		{ -5097,-13252,66676,7258,3234,66626 }, // FCC to SMPTE 240
	};

	if(in->has_sequence_display_extension && in->sd.has_color_description){
		c = in->sd.matrix_coefficients;
	}else{
		if(in->has_sequence_extension){ /* MPEG-2 */
			if(prm->color_matrix){
				c = prm->color_matrix;
			}else{ /* auto select */
				n = rate[in->picture_rate];
				n *= (in->se.frame_rate_ext_n + 1);
				n /= (in->se.frame_rate_ext_d + 1);
				n *= in->orig_h_size * in->orig_v_size;
				if(n < 12000000){ /* ITU-R BT.601 */
					c = 0;
				}else{ /* ITU-R BT.709 */
					c = 1;
				}
			}
		}else{ /* MPEG-1 */
			c = 0;
		}
	}

	memset(out, 0, sizeof(YUY2_CONVERSION_PARAMETER));
	out->uu = 1<<16;
	out->vv = 1<<16;

	switch(prm->yuy2){
	case M2V_CONFIG_YUY2_CONVERT_NONE:
		return 0;
	case M2V_CONFIG_YUY2_CONVERT_BT601:
		switch(c){
		case 1:
			memcpy(out, table+0, sizeof(YUY2_CONVERSION_PARAMETER));
			break;
		case 4:
			memcpy(out, table+1, sizeof(YUY2_CONVERSION_PARAMETER));
			break;
		case 7:
			memcpy(out, table+2, sizeof(YUY2_CONVERSION_PARAMETER));
			break;
		default:
			return 0;
		}
		break;
	case M2V_CONFIG_YUY2_CONVERT_BT709:
		switch(c){
		case 1:
			return 0;
		case 4:
			memcpy(out, table+4, sizeof(YUY2_CONVERSION_PARAMETER));
			break;
		case 7:
			memcpy(out, table+5, sizeof(YUY2_CONVERSION_PARAMETER));
			break;
		default:
			memcpy(out, table+3, sizeof(YUY2_CONVERSION_PARAMETER));
		}
		break;
	case M2V_CONFIG_YUY2_CONVERT_FCC:
		switch(c){
		case 1:
			memcpy(out, table+7, sizeof(YUY2_CONVERSION_PARAMETER));
			break;
		case 4:
			return 0;
		case 7:
			memcpy(out, table+8, sizeof(YUY2_CONVERSION_PARAMETER));
			break;
		default:
			memcpy(out, table+6, sizeof(YUY2_CONVERSION_PARAMETER));
		}
		break;
	case M2V_CONFIG_YUY2_CONVERT_SMPTE240:
		switch(c){
		case 1:
			memcpy(out, table+10, sizeof(YUY2_CONVERSION_PARAMETER));
			break;
		case 4:
			memcpy(out, table+11, sizeof(YUY2_CONVERSION_PARAMETER));
			break;
		case 7:
			return 0;
		default:
			memcpy(out, table+9, sizeof(YUY2_CONVERSION_PARAMETER));
		}
		break;
	default:
		return 0;
	}

	return 1;
}

int sequence_header_to_read_picture_header_option(SEQUENCE_HEADER *in, READ_PICTURE_HEADER_OPTION *out)
{
	if(in->has_sequence_extension){
		out->progressive_sequence = in->se.progressive;
	}else{
		out->progressive_sequence = 1;
	}
	
	return 1;
}

int sequence_header_to_read_slice_header_option(SEQUENCE_HEADER *in, READ_SLICE_HEADER_OPTION *out)
{
	out->v_size = in->v_size;

	if(in->has_sequence_scalable_extension){
		out->scalable_mode = in->ss.scalable_mode;
	}else{
		out->scalable_mode = -1;
	}

	return 1;
}

int sequence_header_to_read_macroblock_option(SEQUENCE_HEADER *in, READ_MACROBLOCK_OPTION *out)
{
	if(in->has_sequence_extension){
		out->mpeg1 = 0;
		out->chroma_format = in->se.chroma_format;
	}else{
		out->mpeg1 = 1;
		out->chroma_format = 1;
	}
	
	if(in->has_sequence_scalable_extension){
		out->scalable_mode = in->ss.scalable_mode;
	}else{
		out->scalable_mode = -1;
	}

	return 1;
}

int sequence_header_to_read_block_option(SEQUENCE_HEADER *in, READ_BLOCK_OPTION *out)
{
	int i;

	if(in->has_sequence_extension){
		out->chroma_format = in->se.chroma_format;
	}else{
		out->chroma_format = 1;
	}
		
	if(in->has_intra_quantizer_matrix){
		for(i=0;i<64;i++){
			out->quantizer_weight[0][scan_table[0][i]] = in->iqm[i];
			out->quantizer_weight[2][scan_table[0][i]] = in->iqm[i];
		}
	}else{
		memcpy(out->quantizer_weight[0], default_intra_quantizer_matrix, sizeof(short)*64);
		memcpy(out->quantizer_weight[2], default_intra_quantizer_matrix, sizeof(short)*64);
	}

	if(in->has_nonintra_quantizer_matrix){
		for(i=0;i<64;i++){
			out->quantizer_weight[1][scan_table[0][i]] = in->nqm[i];
			out->quantizer_weight[3][scan_table[0][i]] = in->nqm[i];
		}
	}else{
		for(i=0;i<64;i++){
			out->quantizer_weight[1][i] = 16;
			out->quantizer_weight[3][i] = 16;
		}
	}

	return 1;
}

int sequence_header_to_mc_parameter(SEQUENCE_HEADER *in, MC_PARAMETER *out)
{
	if(in->has_sequence_extension){
		out->chroma_format = in->se.chroma_format;
	}else{
		out->chroma_format = 1;
	}
	
	return 1;
}
