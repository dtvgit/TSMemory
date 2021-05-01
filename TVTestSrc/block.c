/*******************************************************************
                      block layer interface
 *******************************************************************/

#include "scan.h"
#include "dct_coefficient.h"

#include "block.h"

typedef struct {
	short value;
	short length;
} BASIC_VLC_ELEMENT;

static int get_dct_dc_size_luminance(VIDEO_STREAM *in);
static int get_dct_dc_size_chrominance(VIDEO_STREAM *in);

static __inline int get_dct_dc_diff_luminance(VIDEO_STREAM *in)
{
	int size;
	int diff;

	size = get_dct_dc_size_luminance(in);
	if(size){
		diff = vs_get_bits(in, size);
		if(diff < (1 << (size-1))){
			diff -= (1<<size)-1;
		}
		return diff;
	}else{
		return 0;
	}
}

static __inline int get_dct_dc_diff_chrominance(VIDEO_STREAM *in)
{
	int size;
	int diff;

	size = get_dct_dc_size_chrominance(in);
	if(size){
		diff = vs_get_bits(in, size);
		if(diff < (1 << (size-1))){
			diff -= (1<<size)-1;
		}
		return diff;
	}else{
		return 0;
	}
}

static __inline short limit_dct_coefficient(int w)
{
	if(w >= -2048){
		if(w < 2047){
			return (short)w;
		}else{
			return 2047;
		}
	}
	return -2048;
}

int read_block_null(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt)
{
	return 0;
}

static const short missmatch_control[2][2] = {
	{1, -1}, {0, 0},
};

static const READ_DCT_COEFFICIENT rdc_table[2] = {
	read_dct_ac_coefficient_b14,
	read_dct_ac_coefficient_b15,
};

int read_block_mpeg2_intra_luminance(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt)
{
	int i,n,sum;

	int run;
	int level;

	READ_DCT_COEFFICIENT rdc;
	
	short *qw;
	
	qw = (short *)opt->qw[0];
	
	/* initialize output block */
	memset(out, 0, sizeof(short)*64);

	opt->dc_dct_predictor[0] += get_dct_dc_diff_luminance(in);
	out[0] = limit_dct_coefficient(opt->dc_dct_predictor[0] * (1 << (3-opt->intra_dc_precision)));
	sum = out[0];

	rdc = rdc_table[opt->intra_vlc_format];	
	
	n = 1;
	while(rdc(in, &run, &level) > 0){
		n += run;
		if(n >= 64){
			return 0;
		}
		i = scan_table[opt->alternate_scan][n];

		out[i] = limit_dct_coefficient( (level * qw[i]) / 16 );
		sum += out[i];
		n += 1;
	}

	out[63] += missmatch_control[sum&1][out[63]&1];
	
	return 1;
}

int read_block_mpeg2_intra_cb(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt)
{
	int i,n,sum;

	int run;
	int level;

	READ_DCT_COEFFICIENT rdc;
	
	short *qw;
	
	qw = (short *)opt->qw[2];
	
	/* initialize output block */
	memset(out, 0, sizeof(short)*64);

	opt->dc_dct_predictor[1] += get_dct_dc_diff_chrominance(in);
	out[0] = limit_dct_coefficient(opt->dc_dct_predictor[1] * (1 << (3-opt->intra_dc_precision)));
	sum = out[0];

	rdc = rdc_table[opt->intra_vlc_format];
	
	n = 1;
	while(rdc(in, &run, &level) > 0){
		n += run;
		if(n >= 64){
			return 0;
		}
		i = scan_table[opt->alternate_scan][n];

		out[i] = limit_dct_coefficient( (level * qw[i]) / 16 );
		sum += out[i];
		n += 1;
	}

	out[63] += missmatch_control[sum&1][out[63]&1];
	
	return 1;
}

int read_block_mpeg2_intra_cr(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt)
{
	int i,n,sum;

	int run;
	int level;

	READ_DCT_COEFFICIENT rdc;
	
	short *qw;
	
	qw = (short *)opt->qw[2];
	
	/* initialize output block */
	memset(out, 0, sizeof(short)*64);

	opt->dc_dct_predictor[2] += get_dct_dc_diff_chrominance(in);
	out[0] = limit_dct_coefficient(opt->dc_dct_predictor[2] * (1 << (3-opt->intra_dc_precision)));
	sum = out[0];

	rdc = rdc_table[opt->intra_vlc_format];
	
	n = 1;
	while(rdc(in, &run, &level) > 0){
		n += run;
		if(n >= 64){
			return 0;
		}
		i = scan_table[opt->alternate_scan][n];

		out[i] = limit_dct_coefficient( (level * qw[i]) / 16 );
		sum += out[i];
		n += 1;
	}

	out[63] += missmatch_control[sum&1][out[63]&1];
	
	return 1;
}

int read_block_mpeg2_nonintra_luminance(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt)
{
	int i,n,sum;

	int run;
	int level;

	short *qw;
	
	qw = (short *)opt->qw[1];
	
	/* initialize output block */
	memset(out, 0, sizeof(short)*64);

	/* read first coefficient */
	read_dct_dc_coefficient_b14(in, &run, &level);
	n = run;
	i = scan_table[opt->alternate_scan][n];
	out[i] = limit_dct_coefficient( ( ((level * 2) + ( (level > 0) ? 1 : -1)) * qw[i]) / 32 );
	sum = out[i];
	n += 1;

	while(read_dct_ac_coefficient_b14(in, &run, &level) > 0){
		n += run;
		if(n >= 64){
			return 0;
		}
		i = scan_table[opt->alternate_scan][n];
		out[i] = limit_dct_coefficient( ( ((level * 2) + ( (level > 0) ? 1 : -1)) * qw[i]) / 32 );
		sum += out[i];
		n += 1;
	}

	out[63] += missmatch_control[sum&1][out[63]&1];

	return 1;
}

int read_block_mpeg2_nonintra_chrominance(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt)
{
	int i,n,sum;

	int run;
	int level;

	short *qw;
	
	qw = (short *)opt->qw[3];
	
	/* initialize output block */
	memset(out, 0, sizeof(short)*64);

	/* read first coefficient */
	read_dct_dc_coefficient_b14(in, &run, &level);
	n = run;
	i = scan_table[opt->alternate_scan][n];
	out[i] = limit_dct_coefficient( ( ((level * 2) + ( (level > 0) ? 1 : -1)) * qw[i]) / 32 );
	sum = out[i];
	n += 1;

	while(read_dct_ac_coefficient_b14(in, &run, &level) > 0){
		n += run;
		if(n >= 64){
			return 0;
		}
		i = scan_table[opt->alternate_scan][n];
		out[i] = limit_dct_coefficient( ( ((level * 2) + ( (level > 0) ? 1 : -1)) * qw[i] ) / 32 );
		sum += out[i];
		n += 1;
	}

	out[63] += missmatch_control[sum&1][out[63]&1];

	return 1;
}

int read_block_mpeg1_intra_luminance(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt)
{
	int i,n;

	int run;
	int level;

	short *qw;
	
	qw = (short *)opt->qw[0];
	
	/* initialize output block */
	memset(out, 0, sizeof(short)*64);

	opt->dc_dct_predictor[0] += get_dct_dc_diff_luminance(in);
	out[0] = limit_dct_coefficient(opt->dc_dct_predictor[0] << 3);

	n = 1;
	while(read_dct_ac_coefficient_mpeg1(in, &run, &level) > 0){
		n += run;
		if(n >= 64){
			return 0;
		}
		i = scan_table[0][n];

		out[i] = limit_dct_coefficient( (((level * qw[i]) / 16)-1) | 1);
		n += 1;
	}

	return 1;
}

int read_block_mpeg1_intra_cb(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt)
{
	int i,n;

	int run;
	int level;

	short *qw;
	
	qw = (short *)opt->qw[0];
	
	/* initialize output block */
	memset(out, 0, sizeof(short)*64);

	opt->dc_dct_predictor[1] += get_dct_dc_diff_chrominance(in);
	out[0] = limit_dct_coefficient(opt->dc_dct_predictor[1] << 3);

	n = 1;
	while(read_dct_ac_coefficient_mpeg1(in, &run, &level) > 0){
		n += run;
		if(n >= 64){
			return 0;
		}
		i = scan_table[0][n];

		out[i] = limit_dct_coefficient( (((level * qw[i]) / 16)-1) | 1);
		n += 1;
	}

	return 1;
}

int read_block_mpeg1_intra_cr(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt)
{
	int i,n;

	int run;
	int level;

	short *qw;
	
	qw = (short *)opt->qw[0];
	
	/* initialize output block */
	memset(out, 0, sizeof(short)*64);

	opt->dc_dct_predictor[2] += get_dct_dc_diff_chrominance(in);
	out[0] = limit_dct_coefficient(opt->dc_dct_predictor[2] << 3);

	n = 1;
	while(read_dct_ac_coefficient_mpeg1(in, &run, &level) > 0){
		n += run;
		if(n >= 64){
			return 0;
		}
		i = scan_table[0][n];

		out[i] = limit_dct_coefficient( (((level * qw[i]) / 16)-1) | 1);
		n += 1;
	}

	return 1;
}

int read_block_mpeg1_nonintra(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt)
{
	int i,n;

	int run;
	int level;

	short *qw;
	
	qw = (short *)opt->qw[1];
	
	/* initialize output block */
	memset(out, 0, sizeof(short)*64);

	/* read first coefficient */
	read_dct_dc_coefficient_mpeg1(in, &run, &level);
	n = run;
	i = scan_table[0][n];
	out[i] = limit_dct_coefficient( ((( ((level * 2) + ( (level > 0) ? 1 : -1)) * qw[i] ) / 32)-1)|1 );
	n += 1;

	while(read_dct_ac_coefficient_mpeg1(in, &run, &level) > 0){
		n += run;
		if(n >= 64){
			return 0;
		}
		i = scan_table[0][n];
		out[i] = limit_dct_coefficient( ((( ((level * 2) + ( (level > 0) ? 1 : -1)) * qw[i] ) / 32)-1)|1 );
		n += 1;
	}

	return 1;
}

int reset_dc_dct_predictor(READ_BLOCK_OPTION *p)
{
	int w;

	w = 1 << (7 + p->intra_dc_precision);
	p->dc_dct_predictor[0] = w;
	p->dc_dct_predictor[1] = w;
	p->dc_dct_predictor[2] = w;

	return 1;
}

static int get_dct_dc_size_luminance(VIDEO_STREAM *in)
{
	int code;

	static const BASIC_VLC_ELEMENT table[] = {
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 

		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 

		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 

		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2}, 

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},
		{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},
		{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},
		{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},

		{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},
		{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},
		{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},
		{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},{0,3},

		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},

		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},

		{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},
		{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},
		{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},
		{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},

		{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},
		{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},
		{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},
		{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},{4,3},

		{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},
		{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},
		{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},
		{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},{5,4},

		{6,5},{6,5},{6,5},{6,5},{6,5},{6,5},{6,5},{6,5},
		{6,5},{6,5},{6,5},{6,5},{6,5},{6,5},{6,5},{6,5},
		{7,6},{7,6},{7,6},{7,6},{7,6},{7,6},{7,6},{7,6},
		{8,7},{8,7},{8,7},{8,7},{9,8},{9,8},{10,9},{11,9},
	};

	code = vs_read_bits(in, 9);

	vs_erase_bits(in, table[code].length);
	
	return table[code].value;
}

static int get_dct_dc_size_chrominance(VIDEO_STREAM *in)
{
	int code;

	static const BASIC_VLC_ELEMENT table[] = {
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},

		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},

		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},

		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},

		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},

		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},

		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},

		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},
		{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},{0,2},

		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},{1,2},
		
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},
		{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},{2,2},

		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},

		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},

		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},

		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},
		{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},{3,3},

		{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},
		{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},
		{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},
		{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},

		{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},
		{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},
		{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},
		{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},{4,4},

		{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},
		{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},
		{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},
		{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},{5,5},

		{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},
		{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},{6,6},
		{7,7},{7,7},{7,7},{7,7},{7,7},{7,7},{7,7},{7,7},
		{8,8},{8,8},{8,8},{8,8},{9,9},{9,9},{10,10},{11,10},
	};

	code = vs_read_bits(in, 10);

	vs_erase_bits(in, table[code].length);

	return table[code].value;
}

void __stdcall setup_qw_nosimd(unsigned short *qw, unsigned short *qm, int q)
{
	int i;
	
	for(i=0;i<256;i++){
		qw[i] = qm[i] * q;
	}
}

