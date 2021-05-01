/*******************************************************************
                    Output Buffer Interface
 *******************************************************************/
#ifndef OUT_BUFFER_H
#define OUT_BUFFER_H

#include "frame.h"

#include <windows.h>

typedef struct {
	__int64             index;
	int                 picture_coding_type;
	int                 repeat_first_field;
	int                 top_field_first;
	int                 progressive_frame;
	int                 closed_gop;
} OUTPUT_PARAMETER;

typedef struct {
	OUTPUT_PARAMETER    prm;
	
	int                 ref_count;
	
	FRAME               data;

	void               *prev;
	void               *next;
} OUT_BUFFER_ELEMENT;

typedef struct {
	OUT_BUFFER_ELEMENT *head;
	OUT_BUFFER_ELEMENT *tail;
	int                 count;
} OUT_BUFFER_LIST;

typedef struct {
	
	OUT_BUFFER_LIST     pool;

	OUT_BUFFER_LIST     cache;
	int                 i_frame_count;
	
	OUT_BUFFER_LIST     dec;

	CRITICAL_SECTION    lock;
	
} OUT_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef OUT_BUFFER_C

extern void init_out_buffer(OUT_BUFFER *p);
extern void reset_out_buffer(OUT_BUFFER *p);
extern void release_out_buffer(OUT_BUFFER *p);

extern OUT_BUFFER_ELEMENT *query_work_frame(OUT_BUFFER *p, int width, int height);
extern void back_work_frame(OUT_BUFFER *p, OUT_BUFFER_ELEMENT *elem);
extern void set_decoded_frame(OUT_BUFFER *p, OUT_BUFFER_ELEMENT *elem);
extern OUT_BUFFER_ELEMENT *search_out_buffer(OUT_BUFFER *p, __int64 index);

#endif /* OUT_BUFFER_C */

#ifdef __cplusplus
}
#endif

#endif /* OUT_BUFFER_H */
