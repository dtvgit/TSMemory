/*******************************************************************
                    dct coefficient interface
 *******************************************************************/
#include "video_stream.h"

#ifndef DCT_COEFFICIENT_H
#define DCT_COEFFICIENT_H

typedef int (* READ_DCT_COEFFICIENT)(VIDEO_STREAM *, int *, int *);

#ifdef __cplusplus
extern "C" {
#endif

extern int read_dct_dc_coefficient_b14(VIDEO_STREAM *in, int *run, int *level);
extern int read_dct_dc_coefficient_mpeg1(VIDEO_STREAM *in, int *run, int *level);
extern int read_dct_ac_coefficient_b14(VIDEO_STREAM *in, int *run, int *level);
extern int read_dct_ac_coefficient_mpeg1(VIDEO_STREAM *in, int *run, int *level);
extern int read_dct_ac_coefficient_b15(VIDEO_STREAM *in, int *run, int *level);

#ifdef __cplusplus
}
#endif

#endif
