/*******************************************************************
                       MPEG time code routine
 *******************************************************************/

#define TIMECODE_C
#include "timecode.h"

void read_timecode(VIDEO_STREAM *in, TIMECODE *out);
__int64 timecode2frame(TIMECODE *in, int fps);

void read_timecode(VIDEO_STREAM *in, TIMECODE *out)
{
	out->drop = vs_get_bits(in, 1);
	out->hh = vs_get_bits(in, 5);
	out->mm = vs_get_bits(in, 6);
	out->padding = vs_get_bits(in, 1);
	out->ss = vs_get_bits(in, 6);
	out->ff = vs_get_bits(in, 6);

	return;
}

__int64 timecode2frame(TIMECODE *in, int fps)
{
	__int64 r,w;

	r = in->hh * 60 * 60 * fps;
	r += in->mm * 60 * fps;
	r += in->ss * fps;
	r += in->ff;

	if(in->drop){
		w = in->hh * 60 + in->mm;
		r -= 2 * w;
		r += 2 * (w/10);
	}

	return r;
}