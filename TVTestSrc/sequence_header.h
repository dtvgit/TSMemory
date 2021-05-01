/*******************************************************************
   MPEG Video sequance header interface
 *******************************************************************/

#ifndef SEQUENCE_HEADER_H
#define SEQUENCE_HEADER_H

#include "video_stream.h"
#include "frame.h"
#include "picture_header.h"
#include "slice_header.h"
#include "macroblock.h"
#include "block.h"
#include "mc.h"
#include "config.h"

typedef struct {
	int profile_and_level;
	int progressive;
	int chroma_format;
	int low_delay;
	int frame_rate_ext_n;
	int frame_rate_ext_d;
} SEQUENCE_EXTENSION;

typedef struct {
	int video_format;
	int has_color_description;
	int color_primaries;
	int transfer_characteristics;
	int matrix_coefficients;
	int display_h_size;
	int display_v_size;
} SEQUENCE_DISPLAY_EXTENSION;

typedef struct {
	int scalable_mode;
	int layer;
	int lower_layer_prediction_h_size;
	int lower_layer_prediction_v_size;
	int h_subsampling_facter_m;
	int h_subsampling_facter_n;
	int v_subsampling_facter_m;
	int v_subsampling_facter_n;
	int picture_mux_enable;
	int mux_to_progressive_sequence;
	int pixture_mux_order;
	int pixture_mux_facter;
} SEQUENCE_SCALABLE_EXTENSION;

typedef struct {
	int orig_h_size;
	int orig_v_size;
	int h_size;
	int v_size;
	int aspect_ratio;
	int picture_rate;
	int bit_rate;
	int vbv_buffer_size;
	int mpeg1;
	int has_intra_quantizer_matrix;
	unsigned short iqm[64];
	int has_nonintra_quantizer_matrix;
	unsigned short nqm[64];
	
	int has_sequence_extension;
	SEQUENCE_EXTENSION se;

	int has_sequence_display_extension;
	SEQUENCE_DISPLAY_EXTENSION sd;

	int has_sequence_scalable_extension;
	SEQUENCE_SCALABLE_EXTENSION ss;

} SEQUENCE_HEADER;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SEQUENCE_HEADER_C
extern int read_sequence_header(VIDEO_STREAM *in, SEQUENCE_HEADER *out);
extern int sequence_header_to_bgr_conversion_parameter(SEQUENCE_HEADER *in, BGR_CONVERSION_PARAMETER *out, M2V_CONFIG *prm);
extern int sequence_header_to_yuy2_conversion_parameter(SEQUENCE_HEADER *in, YUY2_CONVERSION_PARAMETER *out, M2V_CONFIG *prm);
extern int sequence_header_to_read_picture_header_option(SEQUENCE_HEADER *in, READ_PICTURE_HEADER_OPTION *out);	
extern int sequence_header_to_read_slice_header_option(SEQUENCE_HEADER *in, READ_SLICE_HEADER_OPTION *out);
extern int sequence_header_to_read_macroblock_option(SEQUENCE_HEADER *in, READ_MACROBLOCK_OPTION *out);
extern int sequence_header_to_read_block_option(SEQUENCE_HEADER *in, READ_BLOCK_OPTION *out);
extern int sequence_header_to_mc_parameter(SEQUENCE_HEADER *in, MC_PARAMETER *out);
#endif

#ifdef __cplusplus
}
#endif

#endif
