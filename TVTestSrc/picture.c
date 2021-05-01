/*******************************************************************
                picture decoding (reconstract) module
 *******************************************************************/
#include <stdio.h>

#define PICTURE_C
#include "picture.h"

/* in block_mmx.asm */
extern void __stdcall copy_i_block_to_frame_mmx(short *in, unsigned char *out, int step);
extern void __stdcall add_diff_to_frame_mmx(short *in, unsigned char *out, int step);

int decode_picture(VIDEO_STREAM *in, MC_BUFFER *out, DECODE_PICTURE_PARAMETER *prm);

void add_block_data_to_frame(short *in, FRAME *out, READ_BLOCK_OPTION *opt, int x, int y, int block_number);
void add_block_data_to_frame_mmx(short *in, FRAME *out, READ_BLOCK_OPTION *opt, int x, int y, int block_number);

static int clamp_int(int val, int min_lim, int max_lim);
static void check_motion_vector(MC_PARAMETER *prm, int x, int y, int width, int height);
static void black_mb(FRAME *out, int x, int y, int chroma_format, int picture_structure);
static void rewind_video_stream(VIDEO_STREAM *in, __int64 picture_header_tail, __int64 rewind_offset);

int decode_picture(VIDEO_STREAM *in, MC_BUFFER *out, DECODE_PICTURE_PARAMETER *prm)
{
	SLICE_HEADER sh;
	MACROBLOCK   mb;

	__declspec(align(16)) short block[64];

	int x, y, address, inc, width, height, max, pos;
	int i;

	__int64 rewind_offset;
	__int64 picture_header_tail;

	address = 0;
	pos = 0;
	width = out->current->width;
	height = out->current->height;
	max = width * height / 16;
	if(prm->mc_parameter.picture_structure != 3){
		max /= 2;
	}

	memset(&sh, 0, sizeof(sh));
	memset(&mb, 0, sizeof(mb));

	rewind_offset = video_stream_tell(in);
	picture_header_tail = rewind_offset;

	while(vs_next_start_code(in)){

		if(!read_slice_header(in, &sh, &(prm->slice_option))){
			/*
			 * error concealment -
			 *   copy MB from forward refernce frame until next picture
			 */
			
			if(address){
				/* back one block */
				address -= 16;
			}
			
			macroblock_to_error_concealment_mc_parameter(&mb, &(prm->mc_parameter));
			
			while(address<max){
				x = address % width;
				y = address / width * 16;
				if(prm->mc_parameter.picture_structure != 3){
					y *= 2;
				}
				if(prm->mc_parameter.macroblock_motion_forward && out->forward == NULL){
				/* no referrence frame */
					black_mb(out->current, x, y, prm->mc_parameter.chroma_format, prm->mc_parameter.picture_structure);
				}else{ /* has referrence frame */
					check_motion_vector(&(prm->mc_parameter), x, y, width, height);
					mc(out, &(prm->mc_parameter), x, y);
				}					
				address += 16;
			}
			
			/* error recovery hack */
			rewind_video_stream(in, picture_header_tail, rewind_offset);
			return 0;
		}

		rewind_offset = video_stream_tell(in);

		slice_header_to_read_block_option(&sh, &(prm->block_option));

		reset_motion_vector_predictor(&mb);

		inc = get_macroblock_address_increment(in);
		
		/* check slice position */
		pos = (sh.slice_vertical_position-1)*width + (inc-1)*16;
		
		if( address == pos ){
			inc = 1;
		}else if( (pos < address) && (address < pos+width) ){
			inc = 1;
			address = pos;
		}else{
			/*
			 * error concealment - 
			 *   copy MB from forward reference frame until current slice
			 */
			macroblock_to_error_concealment_mc_parameter(&mb, &(prm->mc_parameter));

			if( (pos > max) || (pos < address) ){
				pos = max;
			}

			if(address){
				/* back one block */
				address -= 16;
			}
			
			while(address < pos){
				x = address % width;
				y = address / width * 16;
				if(prm->mc_parameter.picture_structure != 3){
					y *= 2;
				}
				if(prm->mc_parameter.macroblock_motion_forward && out->forward == NULL){
					/* no referrence frame */
					black_mb(out->current, x, y, prm->mc_parameter.chroma_format, prm->mc_parameter.picture_structure);
				}else{ /* has referrence frame */
					check_motion_vector(&(prm->mc_parameter), x, y, width, height);
					mc(out, &(prm->mc_parameter), x, y);
				}
				address += 16;
			}

			if(pos == max){
				/* error recovery hack */
				rewind_video_stream(in, picture_header_tail, rewind_offset);
				return 0;
			}

			inc = 1;
		}

		do{
			x = address % width;
			y = address / width * 16;
			if(prm->macroblock_option.picture_structure != 3){
				y *= 2;
			}

			if(!read_macroblock(in, &mb, &(prm->macroblock_option))){
				break;
			}

			if(mb.macroblock_intra == 0){
				macroblock_to_mc_parameter(&mb, &(prm->mc_parameter));
				check_motion_vector(&(prm->mc_parameter), x, y, width, height);
				mc(out, &(prm->mc_parameter), x, y);
			}

			macroblock_to_read_block_option(&mb, &(prm->block_option));
			for(i=0;i<mb.block_count;i++){
				if(mb.read_block[i](in, block, &(prm->block_option))){
					prm->idct_func(block);
					prm->add_block_func(block, out->current, &(prm->block_option), x, y, i);
				}
			}
			address += 16;
			
			if(address >= max){ /* all macroblocks have decoded */
				return 1;
			}
			
			inc = get_macroblock_address_increment(in);
			if(inc == 0){ /* goto next slice */
				break;
			}

			if(inc != 1){ /* skipped macroblock */
				if(prm->macroblock_option.picture_coding_type == 1){
					/* error 
					   intra-picture can't accept skipped macroblock
					 */
					break;
				}
				reset_dc_dct_predictor(&(prm->block_option));
				if(prm->macroblock_option.picture_coding_type == 2){
					reset_motion_vector_predictor(&mb);
					macroblock_to_mc_parameter(&mb, &(prm->mc_parameter));
				}
				if(prm->mc_parameter.picture_structure == 3){
					prm->mc_parameter.prediction_type = PREDICTION_TYPE_FRAME_BASED;
				}else{
					prm->mc_parameter.prediction_type = PREDICTION_TYPE_FIELD_BASED;
					prm->mc_parameter.motion_vertical_field_select[0][0] = (prm->mc_parameter.picture_structure == 2);
					prm->mc_parameter.motion_vertical_field_select[0][1] = (prm->mc_parameter.picture_structure == 2);
				}
			}

			for(i=0;i<inc-1;i++){

				x = address % width;
				y = address / width * 16;
				if(prm->mc_parameter.picture_structure != 3){
					y *= 2;
				}
				check_motion_vector(&(prm->mc_parameter), x, y, width, height);
				mc(out, &(prm->mc_parameter), x, y);

				address += 16;
			}

		}while(1);
	}

	return 1;
}

static const int cc_table[] = {
	0, 0, 0, 0, 1, 2, 1, 2, 1, 2, 1, 2,
};

static const int x_offset[12] = {
	0, 8, 0, 8, 0, 0, 0, 0, 8, 8, 8, 8,
};

static const int y_offset[12][4][2] = {
	{/* block_number - 0 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			0, 0,
		},
		{/* picture_structure - 2 */
			1, 1,
		},
		{/* picture_structure - 3 */
			0, 0,
		},
	},
	{/* block_number - 1 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			0, 0,
		},
		{/* picture_structure - 2 */
			1, 1,
		},
		{/* picture_structure - 3 */
			0, 0,
		},
	},
	{/* block_number - 2 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			16, 16,
		},
		{/* picture_structure - 2 */
			17, 17,
		},
		{/* picture_structure - 3 */
			8, 1,
		},
	},
	{/* block_number - 3 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			16, 16,
		},
		{/* picture_structure - 2 */
			17, 17,
		},
		{/* picture_structure - 3 */
			8, 1,
		},
	},
	{/* block_number - 4 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			0, 0,
		},
		{/* picture_structure - 2 */
			1, 1,
		},
		{/* picture_structure - 3 */
			0, 0,
		},
	},
	{/* block_number - 5 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			0, 0,
		},
		{/* picture_structure - 2 */
			1, 1,
		},
		{/* picture_structure - 3 */
			0, 0,
		},
	},
	{/* block_number - 6 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			16, 16,
		},
		{/* picture_structure - 2 */
			17, 17,
		},
		{/* picture_structure - 3 */
			8, 1,
		},
	},
	{/* block_number - 7 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			16, 16,
		},
		{/* picture_structure - 2 */
			17, 17,
		},
		{/* picture_structure - 3 */
			8, 1,
		},
	},
	{/* block_number - 8 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			0, 0,
		},
		{/* picture_structure - 2 */
			1, 1,
		},
		{/* picture_structure - 3 */
			0, 0,
		},
	},
	{/* block_number - 9 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			0, 0,
		},
		{/* picture_structure - 2 */
			1, 1,
		},
		{/* picture_structure - 3 */
			0, 0,
		},
	},
	{/* block_number - 10 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			16, 16,
		},
		{/* picture_structure - 2 */
			17, 17,
		},
		{/* picture_structure - 3 */
			8, 1,
		},
	},
	{/* block_number - 11 */
		{/* dummy */
			0, 0,
		},
		{/* picture_structure - 1 */
			16, 16,
		},
		{/* picture_structure - 2 */
			17, 17,
		},
		{/* picture_structure - 3 */
			8, 1,
		},
	},
};

static const int y_shift[3][4] = {
	{/* cc == 0, y */
		0, 0, 0, 0,
	},
	{/* cc == 1, u */
		0, 1, 0, 0,
	},
	{/* cc == 2, v */
		0, 1, 0, 0,
	},
};

static const int x_shift[3][4] = {
	{/* cc == 0, y */
		0, 0, 0, 0,
	},
	{/* cc == 1, u */
		0, 1, 1, 0,
	},
	{/* cc == 2, v */
		0, 1, 1, 0,
	}
};

static const int step_shift[3][4][4][2] = {
	{/* cc == 0 */
		{/* dummy */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				0, 0,
			},
			{/* chroma_format == 2 */
				0, 0,
			},
			{/* chroma_format == 3 */
				0, 0,
			},
		},
		{/* picture_structure == 1 */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				1, 1,
			},
			{/* chroma_format == 2 */
				1, 1,
			},
			{/* chroma_format == 3 */
				1, 1,
			},
		},
		{/* picture_structure == 2 */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				1, 1,
			},
			{/* chroma_format == 2 */
				1, 1,
			},
			{/* chroma_format == 3 */
				1, 1,
			},
		},
		{/* picture_structure == 3 */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				0, 1,
			},
			{/* chroma_format == 2 */
				0, 1,
			},
			{/* chroma_format == 3 */
				0, 1,
			},
		},
	},
	{/* cc == 1 */
		{/* dummy */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				0, 0,
			},
			{/* chroma_format == 2 */
				0, 0,
			},
			{/* chroma_format == 3 */
				0, 0,
			},
		},
		{/* picture_structure == 1 */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				1, 1,
			},
			{/* chroma_format == 2 */
				1, 1,
			},
			{/* chroma_format == 3 */
				1, 1,
			},
		},
		{/* picture_structure == 2 */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				1, 1,
			},
			{/* chroma_format == 2 */
				1, 1,
			},
			{/* chroma_format == 3 */
				1, 1,
			},
		},
		{/* picture_structure == 3 */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				0, 0,
			},
			{/* chroma_format == 2 */
				0, 1,
			},
			{/* chroma_format == 3 */
				0, 1,
			},
		},
	},
	{/* cc == 2 */
		{/* dummy */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				0, 0,
			},
			{/* chroma_format == 2 */
				0, 0,
			},
			{/* chroma_format == 3 */
				0, 0,
			},
		},
		{/* picture_structure == 1 */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				1, 1,
			},
			{/* chroma_format == 2 */
				1, 1,
			},
			{/* chroma_format == 3 */
				1, 1,
			},
		},
		{/* picture_structure == 2 */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				1, 1,
			},
			{/* chroma_format == 2 */
				1, 1,
			},
			{/* chroma_format == 3 */
				1, 1,
			},
		},
		{/* picture_structure == 3 */
			{/* dummy */
				0, 0,
			},
			{/* chroma_format == 1 */
				0, 0,
			},
			{/* chroma_format == 2 */
				0, 1,
			},
			{/* chroma_format == 3 */
				0, 1,
			},
		},
	},
};

void add_block_data_to_frame(short *in, FRAME *out, READ_BLOCK_OPTION *opt, int x, int y, int block_number)
{
	int i,j;
	int step, cc;
	unsigned char *p;
	
	cc = cc_table[block_number];
	
	switch(cc){
	case 0:
		p = out->y;
		break;
	case 1:
		p = out->u;
		break;
	case 2:
		p = out->v;
		break;
	default:
		return;    
	};

	x >>= x_shift[cc][opt->chroma_format];
	y >>= y_shift[cc][opt->chroma_format];
	step = out->width << step_shift[cc][opt->picture_structure][opt->chroma_format][opt->dct_type];
	p += (y + y_offset[block_number][opt->picture_structure][opt->dct_type]) * out->width;
	p += x + x_offset[block_number];
	
	if(opt->macroblock_intra){
		for(i=0;i<8;i++){
			for(j=0;j<8;j++){
				p[j] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+in[j]];
			}
			p += step;
			in += 8;
		}
	}else{
		for(i=0;i<8;i++){
			for(j=0;j<8;j++){
				p[j] = uchar_clip_table[UCHAR_CLIP_TABLE_OFFSET+p[j]+in[j]];
			}
			p += step;
			in += 8;
		}
	}
}

typedef void ( __stdcall *ADD_BLOCK_CORE)(short *, unsigned char *, int);

void add_block_data_to_frame_mmx(short *in, FRAME *out, READ_BLOCK_OPTION *opt, int x, int y, int block_number)
{
	int step, cc;
	unsigned char *p;

	static const ADD_BLOCK_CORE table[2] = {
		add_diff_to_frame_mmx, copy_i_block_to_frame_mmx,
	};
	
	cc = cc_table[block_number];
	
	switch(cc){
	case 0:
		p = out->y;
		break;
	case 1:
		p = out->u;
		break;
	case 2:
		p = out->v;
		break;
	default:
		return;    
	};

	x >>= x_shift[cc][opt->chroma_format];
	y >>= y_shift[cc][opt->chroma_format];
	step = out->width << step_shift[cc][opt->picture_structure][opt->chroma_format][opt->dct_type];
	p += (y + y_offset[block_number][opt->picture_structure][opt->dct_type]) * out->width;
	p += x + x_offset[block_number];
	
	table[opt->macroblock_intra](in, p, step);
}

static int clamp_int(int val, int min_lim, int max_lim)
{
	if (val < min_lim) { return min_lim; }
	if (val > max_lim) { return max_lim; }
	return val;
}

static void check_motion_vector(MC_PARAMETER *prm, int x, int y, int width, int height)
{
	int r,s;
	int r_start,r_last;
	int s_start,s_last;
	int hrz_min,hrz_max;
	int vrt_min,vrt_max;

	r_start = 0;
	r_last = 1;
	if (((prm->picture_structure == 3) && (prm->prediction_type != PREDICTION_TYPE_FRAME_BASED)) ||
	    ((prm->picture_structure != 3) && (prm->prediction_type == PREDICTION_TYPE_16x8_MC)) ){
		r_last = 2;
	}

	s_start = 1;
	if (prm->macroblock_motion_forward) {
		s_start = 0;
	}

	s_last = 1;
	if (prm->macroblock_motion_backward) {
		s_last = 2;
	}
	
	hrz_min = -2 * x;
	hrz_max = (width - x - 16) * 2;

	vrt_min = -2 * y;
	if (prm->picture_structure == 3) {
		/* a frame coded picture */
		vrt_max = (height - y - 16) * 2;
	} else {
		/* a field coded picture */
		vrt_max = (height - y - 32) * 2;
		if (prm->prediction_type == PREDICTION_TYPE_16x8_MC) {
			vrt_max += 32;
		}
	}

	for (r=r_start;r<r_last;r++) {
		if (prm->prediction_type == PREDICTION_TYPE_16x8_MC) {
			/* a bottom 16x8 block has vertical offsets */
			vrt_min -= (r << 5); 
			vrt_max -= (r << 5);
		}
		for (s=s_start;s<s_last;s++) {
			prm->PMV[r][s][0] = clamp_int(prm->PMV[r][s][0], hrz_min, hrz_max);
			prm->PMV[r][s][1] = clamp_int(prm->PMV[r][s][1], vrt_min, vrt_max);
		}
		if (prm->prediction_type == PREDICTION_TYPE_DUAL_PRIME) {
			prm->DMV[r][0] = clamp_int(prm->DMV[r][0], hrz_min, hrz_max);
			prm->DMV[r][1] = clamp_int(prm->DMV[r][1], vrt_min/2, vrt_max/2);
		}
	}
}

static void black_mb(FRAME *out, int x, int y, int chroma_format, int picture_structure)
{
	int i,j;
	int step, offset;
	int bx,by;

	unsigned char *p;

	step = out->width;
	offset = 0;
	if(picture_structure != 3){
		step += out->width;
		if(picture_structure == 2){
			offset = out->width;
		}
	}

	/* for Y */
	p = out->y + (y * out->width + x) + offset;

	for(i=0;i<16;i++){
		for(j=0;j<16;j++){
			p[j] = 16;
		}
		p += step;
	}
	
	switch(chroma_format){
	case 1: /* 420 */
		bx = 8;
		by = 8;
		y >>= 1;
		x >>= 1;
		break;
	case 2: /* 422 */
		bx = 8;
		by = 16;
		x >>= 1;
		break;
	case 3: /* 444 */
	default:
		bx = 16;
		by = 16;
		break;
	}

	/* for Cb */
	p = out->u + (y * out->width + x) + offset;
	for(i=0;i<by;i++){
		for(j=0;j<bx;j++){
			p[j] = 128;
		}
		p += step;
	}

	/* for Cr */
	p = out->v + (y * out->width + x) + offset;
	for(i=0;i<by;i++){
		for(j=0;j<bx;j++){
			p[j] = 128;
		}
		p += step;
	}
}

static void rewind_video_stream(VIDEO_STREAM *in, __int64 picture_header_tail, __int64 rewind_offset)
{
	__int64 cur;

	cur = video_stream_seek(in, rewind_offset, SEEK_SET);
	if(cur >= picture_header_tail){
		return;
	}

	while(vs_next_start_code(in)){
		if(video_stream_tell(in) >= picture_header_tail){
			break;
		}
		vs_erase_bits(in, 32);
	}
}

