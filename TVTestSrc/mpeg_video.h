/*******************************************************************
                   MPEG Video Decoding Interface
 *******************************************************************/
#ifndef MPEG_VIDEO_H
#define MPEG_VIDEO_H

#include <windows.h>

#include "video_stream.h"
#include "sequence_header.h"
#include "picture_header.h"
#include "frame.h"
#include "gop.h"
#include "picture.h"
#include "mc.h"
#include "idct.h"
#include "out_buffer.h"
#include "config.h"
#include "resize.h"

typedef struct {
	VIDEO_STREAM                bitstream;
	SEQUENCE_HEADER             seq;
	PICTURE_HEADER              pic;
	READ_PICTURE_HEADER_OPTION  pic_opt;

	FIND_GOP                    fg;
	GOP                         current;
	
	MC_BUFFER                   dec_buf;
	DECODE_PICTURE_PARAMETER   *dec_prm;

	M2V_CONFIG                  config;

	int                         closed_gop;

	int                         orig_field_order;
	int                         disp_field_order;

	RESIZE_PARAMETER           *rsz_prm;
	
	BGR_CONVERSION_PARAMETER    bgr_prm;
	YUY2_CONVERSION_PARAMETER   ycc_prm;

	__int64                     fwd_index;
	__int64                     cur_index;
	__int64                     bwd_index;

	OUT_BUFFER_ELEMENT         *fwd;
	OUT_BUFFER_ELEMENT         *cur;
	OUT_BUFFER_ELEMENT         *bwd;

	OUT_BUFFER                  out_buf;
	
	YUV_to_BGR                  to_bgr;
	YUV_to_YUY2                 to_yuy2;
	YUY2_CONVERT                yuy2_cc;
	UPSAMPLE_CHROMA             upsmp_c[2];

	__int64                     total;
	int                         rate;
	int                         scale;

	int                         width;
	int                         height;

	CRITICAL_SECTION            lock;

	HANDLE                      req_event;
	HANDLE                      dec_event;

	HANDLE                      thrd;
	unsigned int                thid;
	int                         thrd_stat;
	
} MPEG_VIDEO;
	
#ifdef __cplusplus
extern "C" {
#endif

#ifndef MPEG_VIDEO_C	
extern MPEG_VIDEO *open_mpeg_video(char *path);
extern int close_mpeg_video(MPEG_VIDEO *p);
extern OUT_BUFFER_ELEMENT *read_frame(MPEG_VIDEO *in, __int64 frame);
extern int get_picture_coding_type(MPEG_VIDEO *in, __int64 frame);
#endif

#ifdef __cplusplus
}
#endif

#endif
