/*******************************************************************
                      block layer interface
 *******************************************************************/
#ifndef BLOCK_H
#define BLOCK_H

#include "video_stream.h"

typedef struct {
	/* in sequence_header &
	      picture_header ->
	        quant_matrix_extension */
	unsigned short quantizer_weight[4][64];

	/* work area */
	unsigned short qw_table[32][4][64];
	unsigned short *qw[4];

	/* func pointer */
	void (__stdcall * setup_qw)(unsigned short *qw, unsigned short *qm, int q);

	/* in sequence_header ->
	        sequence_extension */ 
	int chroma_format;

	/* in picture_header ->
	        picture_coding_extension */
	int picture_structure;
	int intra_dc_precision;
	int intra_vlc_format;
	int alternate_scan;
	int q_scale_type;
	
	/* in macroblock */
	int macroblock_intra;
	int dct_type;
	int dc_dct_predictor[3];

} READ_BLOCK_OPTION;

typedef int (* READ_BLOCK)(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);

#ifdef __cplusplus
extern "C" {
#endif

extern int read_block_null(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);
extern int read_block_mpeg2_intra_luminance(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);
extern int read_block_mpeg2_intra_cb(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);
extern int read_block_mpeg2_intra_cr(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);
extern int read_block_mpeg2_nonintra_luminance(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);
extern int read_block_mpeg2_nonintra_chrominance(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);
extern int read_block_mpeg1_intra_luminance(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);
extern int read_block_mpeg1_intra_cb(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);
extern int read_block_mpeg1_intra_cr(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);
extern int read_block_mpeg1_nonintra(VIDEO_STREAM *in, short *out, READ_BLOCK_OPTION *opt);
extern int reset_dc_dct_predictor(READ_BLOCK_OPTION *p);
extern void __stdcall setup_qw_nosimd(unsigned short *qw, unsigned short *qm, int q);
extern void __stdcall setup_qw_mmx(unsigned short *qw, unsigned short *qm, int q);
extern void __stdcall setup_qw_sse2(unsigned short *qw, unsigned short *qm, int q);

#ifdef __cplusplus
}
#endif

#endif

