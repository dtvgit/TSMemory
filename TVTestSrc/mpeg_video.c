/*******************************************************************
                    MPEG Video Decoding Module
 *******************************************************************/
#include <stdio.h>
#include <process.h>

#include "gop_list.h"
#include "filename.h"
#include "registry.h"
#include "gl_dialog.h"

#include "instance_manager.h"

#include "idct_llm_int.h"
#include "idct_llm_mmx.h"
#include "idct_reference.h"
#include "idct_reference_sse.h"
#include "idct_ap922_int.h"
#include "idct_ap922_mmx.h"
#include "idct_ap922_sse.h"
#include "idct_ap922_sse2.h"

#define MPEG_VIDEO_C
#include "mpeg_video.h"

/* grobal */
MPEG_VIDEO *open_mpeg_video(char *path);
void close_mpeg_video(MPEG_VIDEO *p);
OUT_BUFFER_ELEMENT *read_frame(MPEG_VIDEO *in, __int64 frame);
int get_picture_coding_type(MPEG_VIDEO *in, __int64 frame);
__int64 get_next_i_picture(MPEG_VIDEO *in, __int64 frame);
__int64 get_prev_i_picture(MPEG_VIDEO *in, __int64 frame);

/* local */
static int setup_thread_resource(MPEG_VIDEO *in);
static void release_thread_resource(MPEG_VIDEO *in);
static unsigned int __stdcall decode_thread(void *arg);
static void pause_decode_thread(MPEG_VIDEO *in);
static void resume_decode_thread(MPEG_VIDEO *in);
static void pause_decoding(MPEG_VIDEO *in);
static void decode_one_frame(MPEG_VIDEO *in);

static __inline int prepare_pic_decode(MPEG_VIDEO *in);

static OUT_BUFFER_ELEMENT *proc_sequence_header(MPEG_VIDEO *in);
static OUT_BUFFER_ELEMENT *proc_gop_header(MPEG_VIDEO *in);
static OUT_BUFFER_ELEMENT *proc_picture_data(MPEG_VIDEO *in);

static OUT_BUFFER_ELEMENT *store_work_frame(MPEG_VIDEO *in, int type);
static void cancel_work_frame(MPEG_VIDEO *in, int type);
static void release_work_frame(MPEG_VIDEO *in, int type);
static void setup_current_frame(MPEG_VIDEO *in);
static OUT_BUFFER_ELEMENT *rotate_reference_frame(MPEG_VIDEO *in);
static int is_seek_required(MPEG_VIDEO *p, __int64 frame);

static void sequence_header_to_decode_picture_parameter(SEQUENCE_HEADER *in, DECODE_PICTURE_PARAMETER *out);
static void picture_header_to_decode_picture_parameter(PICTURE_HEADER *in, DECODE_PICTURE_PARAMETER *out);

static void decode_2nd_field(MPEG_VIDEO *p);

static int is_registered_suffix(char *filepath);

static void reset_working_frames(MPEG_VIDEO *p);

static void setup_m2v_config(M2V_CONFIG *p);

static void verify_sequence_size(MPEG_VIDEO *in);
static void verify_sequence_header(MPEG_VIDEO *in, __int64 frame);

static void setup_chroma_upsampling_function(MPEG_VIDEO *p, int chroma_format, int simd);
static void setup_convert_function(MPEG_VIDEO *p, int chroma_format, int simd);
static void setup_qw_function(DECODE_PICTURE_PARAMETER *p, int simd);
static void setup_idct_function(DECODE_PICTURE_PARAMETER *p, M2V_CONFIG *prm);
static void setup_mc_function(DECODE_PICTURE_PARAMETER *p, M2V_CONFIG *prm);
static void setup_add_block_function(DECODE_PICTURE_PARAMETER *p, M2V_CONFIG *prm);
static void setup_field_order(MPEG_VIDEO *p, M2V_CONFIG *prm);
static void resize_parameter_to_bgr_conversion_parameter(RESIZE_PARAMETER *in, BGR_CONVERSION_PARAMETER *out);
static YUY2_CONVERT select_yuy2_convert_function(int simd);

/* local constants */
enum THREAD_STATUS {
	THREAD_STAT_RUNNING  =  0,
	THREAD_STAT_PAUSING  =  1,
	THREAD_STAT_PAUSED   =  2,
	THREAD_STAT_STOPPING =  3,
	THREAD_STAT_STOPPED  =  4,
	THREAD_STAT_ERROR    = -1,
};

enum WORK_FRAME_TYPE {
	WORK_FRAME_FWD       = 0x01,
	WORK_FRAME_CUR       = 0x02,
	WORK_FRAME_BWD       = 0x04,
	WORK_FRAME_ALL       = 0x07,
};

/******************************************************************************/
MPEG_VIDEO *open_mpeg_video(char *path)
{
	int code;
	__int64 limit;
	
	GOP g;
	READ_GOP_PARAMETER *gp;
	GOP_LIST *gl;

	MPEG_VIDEO *out;

	char gl_path[FILENAME_MAX]; /* GOP LIST file path */

	/* initialize */
	out = (MPEG_VIDEO *)calloc(1, sizeof(MPEG_VIDEO)+sizeof(DECODE_PICTURE_PARAMETER)+16);
	if(out == NULL){
		return NULL;
	}

	limit = (__int64)(out+1);
	limit += 16;
	limit &= (-16);
	out->dec_prm = (DECODE_PICTURE_PARAMETER *)limit;
	memset(out->dec_prm, 0, sizeof(DECODE_PICTURE_PARAMETER));
	
	code = 0;

	/* start file check */

	if(!is_registered_suffix(path)){
		free(out);
		return NULL;
	}

	if(!open_video_stream(path, &(out->bitstream))){
		/* can't open */
		free(out);
		return NULL;
	}

	limit = get_filecheck_limit();
	limit += video_stream_tell(&(out->bitstream));
	while(vs_next_start_code(&(out->bitstream))){
		code = vs_get_bits(&(out->bitstream), 32);
		if(code == 0x1B3){
			break;
		}else if(limit < video_stream_tell(&(out->bitstream))){
			/* limit over */
			break;
		}
	}

	if(code != 0x1B3){
		/* failed to find sequence_start_code */
		close_video_stream(&(out->bitstream));
		free(out);
		return 0;
	}

	if(!read_sequence_header(&(out->bitstream), &(out->seq))){
		/* has invalid sequence header */
		close_video_stream(&(out->bitstream));
		free(out);
		return 0;
	}

	sequence_header_to_read_picture_header_option(&(out->seq), &(out->pic_opt));
	sequence_header_to_decode_picture_parameter(&(out->seq), out->dec_prm);

	while(vs_next_start_code(&(out->bitstream))){
		code = vs_get_bits(&(out->bitstream), 32);
		if(code == 0x100){
			break;
		}else if(limit < video_stream_tell(&(out->bitstream))){
			/* limit over */
			break;
		}
	}
	if(code != 0x100){
		/* failed to find picture_start_code */
		close_video_stream(&(out->bitstream));
		free(out);
		return 0;
	}
	
	if(!read_picture_header(&(out->bitstream), &(out->pic), &(out->pic_opt))){
		/* has invalid picture header */
		close_video_stream(&(out->bitstream));
		free(out);
		return 0;
	}

	/* finish file check */
	
	setup_m2v_config(&(out->config));

	out->orig_field_order = out->pic.pc.top_field_first;
	
	video_stream_seek(&(out->bitstream), 0, SEEK_SET);
	
	gp = new_read_gop_parameter(&(out->bitstream), &(out->seq), &(out->pic_opt), out->orig_field_order);
	if(gp == NULL){
		/* malloc failed */
		close_video_stream(&(out->bitstream));
		free(out);
		return 0;
	}

	out->rate = gp->rate;
	out->scale = gp->scale;

	if( (out->config.gl & M2V_CONFIG_GL_ALWAYS_SCAN) == 0){
		if(read_gop(gp, &g)){
			/* gop timecode is sequential */
			out->fg.arg1 = (void *)gp;
			out->fg.func = find_gop_with_timecode;
			out->fg.release = delete_read_gop_parameter;
			
			out->total = count_frame(gp);
		}
	}

	if(out->total <= 0){
		/* gop timecode is not sequential */
		/* or GL_ALWAYS_SCAN selected     */
		delete_read_gop_parameter(gp);
		
		if(out->config.gl & M2V_CONFIG_GL_NEVER_SAVE){
			video_stream_seek(&(out->bitstream), 0, SEEK_SET);
			gl = new_gop_list(&(out->bitstream), &(out->pic_opt), out->orig_field_order, GL_DIALOG_MODE_GUI);
			if(gl == NULL){
				/* stream has something probrem */
				close_video_stream(&(out->bitstream));
				return 0;
			}
		}else{
			strcpy(gl_path, path);
			cut_suffix(gl_path);
			strcat(gl_path, ".gl");

			gl = load_gop_list(gl_path);
			if( (gl == NULL) || (gl->stream_length != out->bitstream.file_length) ){
				video_stream_seek(&(out->bitstream), 0, SEEK_SET);
				gl = new_gop_list(&(out->bitstream), &(out->pic_opt), out->orig_field_order, GL_DIALOG_MODE_GUI);
				if(gl == NULL){
					/* stream has something probrem */
					close_video_stream(&(out->bitstream));
					return 0;
				}
				store_gop_list(gl, gl_path);
			}
		}
		out->fg.arg1 = (void *) gl;
		out->fg.func = find_gop_with_gop_list;
		out->fg.release = delete_gop_list;

		out->total = gl->num_of_frame;
	}

	verify_sequence_size(out);

	sequence_header_to_bgr_conversion_parameter(&(out->seq), &(out->bgr_prm), &(out->config));
	if(sequence_header_to_yuy2_conversion_parameter(&(out->seq), &(out->ycc_prm), &(out->config))){
		out->yuy2_cc = select_yuy2_convert_function(out->config.simd);
	}else{
		out->yuy2_cc = yuy2_convert_none;
	}

	if( (out->seq.has_sequence_extension == 0) || (out->seq.se.progressive) ){
		out->config.field_mode = M2V_CONFIG_FRAME_KEEP_ORIGINAL;
	}
	setup_field_order(out, &(out->config));

	setup_idct_function(out->dec_prm, &(out->config));
	setup_mc_function(out->dec_prm, &(out->config));
	setup_add_block_function(out->dec_prm, &(out->config));

	out->rsz_prm = create_resize_parameter(&(out->seq), &(out->config));
	resize_parameter_to_bgr_conversion_parameter(out->rsz_prm, &(out->bgr_prm));

	if(out->seq.has_sequence_extension){
		setup_chroma_upsampling_function(out, out->seq.se.chroma_format, out->config.simd);
		setup_convert_function(out, out->seq.se.chroma_format, out->config.simd);
	}else{
		setup_chroma_upsampling_function(out, 1, out->config.simd);
		setup_convert_function(out, 1, out->config.simd);
	}
	setup_qw_function(out->dec_prm, out->config.simd);

	out->width = out->bgr_prm.prm.width;
	out->height = out->bgr_prm.prm.height;

	video_stream_seek(&(out->bitstream), 0, SEEK_SET);

	out->dec_buf.forward = NULL;
	out->dec_buf.backward = NULL;
	out->dec_buf.current = NULL;

	out->current.frame_count = 0;
	out->current.start_frame = -1;

	init_out_buffer(&(out->out_buf));
	
	reset_working_frames(out);

	InitializeCriticalSection(&(out->lock));

	if (!setup_thread_resource(out)) {
		close_mpeg_video(out);
		return 0;
	}

	register_instance(out, (TEARDOWN_PROC)close_mpeg_video);

	return out;
}
	
void close_mpeg_video(MPEG_VIDEO *p)
{
	if(p == NULL){
		return;
	}

	EnterCriticalSection(&(p->lock));

	remove_instance(p);

	release_thread_resource(p);

	release_out_buffer(&(p->out_buf));

	memset(&(p->dec_buf), 0, sizeof(MC_BUFFER));
	
	if(p->fg.release){
		p->fg.release(p->fg.arg1);
		p->fg.release = NULL;
	}

	release_resize_parameter(p->rsz_prm);
	p->rsz_prm = NULL;
	
	close_video_stream(&(p->bitstream));

	LeaveCriticalSection(&(p->lock));
	DeleteCriticalSection(&(p->lock));

	free(p);
}

OUT_BUFFER_ELEMENT *read_frame(MPEG_VIDEO *in,__int64 frame)
{
	OUT_BUFFER_ELEMENT *r;
	r = NULL;

	r = search_out_buffer(&(in->out_buf), frame);
	if (in->req_event != NULL) {
		SetEvent(in->req_event);
	}
	if (r != NULL) {
		goto LAST;
	}

	if (is_seek_required(in, frame)) {
		
		pause_decode_thread(in);

		in->current = in->fg.func(in->fg.arg1, frame);
		video_stream_seek(&(in->bitstream), in->current.offset, SEEK_SET);

		reset_working_frames(in);

		if ((in->current.frame_count == 0) ||
		    (in->current.start_frame > frame)) {
			/* failed on seek */
			goto LAST;
		}

		reset_out_buffer(&(in->out_buf));
		if (in->req_event != NULL) {
			SetEvent(in->req_event);
		}

		if(in->current.sh){
			if( (in->seq.orig_h_size != in->current.sh->orig_h_size) || (in->seq.orig_v_size != in->current.sh->orig_v_size) ){
				release_resize_parameter(in->rsz_prm);
				in->rsz_prm = create_force_resize_parameter(in->current.sh, in->width, in->height);
			}
			memcpy(&(in->seq), in->current.sh, sizeof(SEQUENCE_HEADER));
			sequence_header_to_bgr_conversion_parameter(&(in->seq), &(in->bgr_prm), &(in->config));
			if(sequence_header_to_yuy2_conversion_parameter(&(in->seq), &(in->ycc_prm), &(in->config))){
				in->yuy2_cc = select_yuy2_convert_function(in->config.simd);
			}else{
				in->yuy2_cc = yuy2_convert_none;
			}
			sequence_header_to_read_picture_header_option(&(in->seq), &(in->pic_opt));
			sequence_header_to_decode_picture_parameter(&(in->seq), in->dec_prm);
			resize_parameter_to_bgr_conversion_parameter(in->rsz_prm, &(in->bgr_prm));
		}
	}

	if (in->thrd_stat != THREAD_STAT_RUNNING) {
		resume_decode_thread(in);
	}

	r = search_out_buffer(&(in->out_buf), frame);
	if (in->req_event != NULL) {
		SetEvent(in->req_event);
	}
	
	while (r == NULL) {
		
		LeaveCriticalSection(&(in->lock));
		WaitForSingleObject(in->dec_event, 100);
		EnterCriticalSection(&(in->lock));
		
		r = search_out_buffer(&(in->out_buf), frame);
		if (in->req_event != NULL) {
			SetEvent(in->req_event);
		}
		
		if (r != NULL) {
			goto LAST;
		}

		if (in->thrd_stat != THREAD_STAT_RUNNING){
			goto LAST;
		}
	}
	
LAST:
	return r;
}

int get_picture_coding_type(MPEG_VIDEO *in, __int64 frame)
{
	int r;
	OUT_BUFFER_ELEMENT *e;
	
	e = read_frame(in, frame);
	if(e == NULL){
		return 0;
	}

	r = e->prm.picture_coding_type;
	if( (r == 3) && e->prm.closed_gop ){
		r = 4;
	}
	
	return r;
}

__int64 get_next_i_picture(MPEG_VIDEO *in, __int64 frame)
{
	__int64 r;
	__int64 save_offset;
	GOP gop;
	
	pause_decode_thread(in);

	save_offset = video_stream_tell(&(in->bitstream));
	if (frame < 0) {
		frame = 0;
	} else if (frame >= in->total) {
		frame = in->total - 1;
	}
	
	gop = in->fg.func(in->fg.arg1, frame);
	r = gop.start_frame + gop.frame_count;
	if (r >= in->total) {
		r = gop.start_frame;
	}

	video_stream_seek(&(in->bitstream), save_offset, SEEK_SET);

	resume_decode_thread(in);

	return r;
}

__int64 get_prev_i_picture(MPEG_VIDEO *in, __int64 frame)
{
	__int64 r;
	__int64 save_offset;
	GOP gop;
	
	pause_decode_thread(in);

	save_offset = video_stream_tell(&(in->bitstream));
	
	frame -= 1;
	if (frame < 0) {
		frame = 0;
	} else if (frame >= in->total) {
		frame = in->total - 1;
	}
	
	gop = in->fg.func(in->fg.arg1, frame);
	r = gop.start_frame;

	video_stream_seek(&(in->bitstream), save_offset, SEEK_SET);

	resume_decode_thread(in);

	return r;
}

static int setup_thread_resource(MPEG_VIDEO *in)
{
	in->req_event = CreateEvent(NULL, 0, 0, NULL);
	in->dec_event = CreateEvent(NULL, 0, 0, NULL);

	if ((in->req_event == NULL) ||
	    (in->dec_event == NULL)) {
		return 0;
	}

	in->thrd_stat = THREAD_STAT_PAUSING;

	in->thrd = (HANDLE)_beginthreadex(NULL, 0, decode_thread, in, 0, &(in->thid));
	if (in->thrd == NULL) {
		return 0;
	}

	SetThreadPriority(in->thrd, THREAD_PRIORITY_ABOVE_NORMAL);

	return 1;
}

static void release_thread_resource(MPEG_VIDEO *in)
{
	if (in->thrd != NULL) {
		in->thrd_stat = THREAD_STAT_STOPPING;
		SetEvent(in->req_event);

		LeaveCriticalSection(&(in->lock));
		while ( (GetCurrentThreadId() != in->thid) &&
			(in->thrd_stat != THREAD_STAT_STOPPED) &&
			(WaitForSingleObject(in->thrd, 0) != WAIT_OBJECT_0) ) {
			// 待機条件詳細
			//
			// スレッドを WaitForSingleObject すると、DllMain() から
			// ココに来た場合、THREAD_DETACH が DllMain() 待ちで
			// 完了せずにデッドロックしてしまう
			//
			// 状態が停止になるか、スレッドが終了するかどちらかを
			// 終了条件として (スレッド ID 比較は為念) デコード
			// イベントを待つことにする
			WaitForSingleObject(in->dec_event, 100);
		}
		EnterCriticalSection(&(in->lock));
		CloseHandle(in->thrd);
		in->thrd = NULL;
		in->thid = 0;
	}
	
	if (in->req_event != NULL) {
		CloseHandle(in->req_event);
		in->req_event = NULL;
	}

	if (in->dec_event != NULL) {
		CloseHandle(in->dec_event);
		in->dec_event = NULL;
	}
}

static unsigned int __stdcall decode_thread(void *arg)
{
	MPEG_VIDEO *p = (MPEG_VIDEO *)arg;

	while ( (p->thrd_stat != THREAD_STAT_STOPPING) &&
		(p->thrd_stat != THREAD_STAT_ERROR) ) {

		if ((p->thrd_stat == THREAD_STAT_PAUSING) ||
		    (p->thrd_stat == THREAD_STAT_PAUSED)) {
			pause_decoding(p);
			continue;
		}

		decode_one_frame(p);
	}

	p->thrd_stat = THREAD_STAT_STOPPED;

	if (p->dec_event != NULL) {
		SetEvent(p->dec_event);
	}

	return 0;
}

static void pause_decode_thread(MPEG_VIDEO *in)
{
	if (in->thrd_stat == THREAD_STAT_RUNNING) {
		in->thrd_stat = THREAD_STAT_PAUSING;
	}

	if (in->req_event != NULL) {
		SetEvent(in->req_event);
	}

	while (in->thrd_stat == THREAD_STAT_PAUSING) {
		LeaveCriticalSection(&(in->lock));
		WaitForSingleObject(in->dec_event, 100);
		EnterCriticalSection(&(in->lock));
	}
}

static void resume_decode_thread(MPEG_VIDEO *in)
{
	if (in->thrd_stat == THREAD_STAT_PAUSED) {
		in->thrd_stat = THREAD_STAT_RUNNING;
	}
	
	if (in->req_event != NULL) {
		SetEvent(in->req_event);
	}
}

static void pause_decoding(MPEG_VIDEO *in)
{
	if (in->thrd_stat == THREAD_STAT_PAUSING) {
		in->thrd_stat = THREAD_STAT_PAUSED;
	}

	if (in->dec_event != NULL) {
		SetEvent(in->dec_event);
	}

	while (in->thrd_stat == THREAD_STAT_PAUSED) {
		WaitForSingleObject(in->req_event, 100);
	}
}

static void decode_one_frame(MPEG_VIDEO *in)
{
	int code;
	__int64 offset;
	OUT_BUFFER_ELEMENT *r;

	r = NULL;
	while (vs_next_start_code(&(in->bitstream))) {
		offset = video_stream_tell(&(in->bitstream));
		code = vs_get_bits(&(in->bitstream), 32);

		if(code == 0x1B3){
			r = proc_sequence_header(in);
		}else if(code == 0x1B8){
			r = proc_gop_header(in);
		}else if(code == 0x100){
			r = proc_picture_data(in);
		}
		
		if (r != NULL) {
			goto LAST;
		}
	}

	if (in->thrd_stat == THREAD_STAT_RUNNING) {
		in->thrd_stat = THREAD_STAT_PAUSED;
	}

	if (in->dec_buf.forward != NULL) {
		cancel_work_frame(in, WORK_FRAME_FWD);
	}

	if (in->dec_buf.backward != NULL) {
		store_work_frame(in, WORK_FRAME_BWD);
		cancel_work_frame(in, WORK_FRAME_BWD);
	}

LAST:
	if (in->dec_event != NULL) {
		SetEvent(in->dec_event);
	}
}

static __inline int prepare_pic_decode(MPEG_VIDEO *in)
{
	if (in->pic.picture_coding_type == 3) {
		if (in->dec_buf.backward == NULL) {
			return 0;
		}

		if (in->dec_buf.forward == NULL) {
			if (in->closed_gop == 0) {
				return 0;
			}
			/* closed_gop error concealment */
			in->dec_buf.forward = in->dec_buf.backward;
		}
	} else if (in->pic.picture_coding_type == 2) {
		if (in->dec_buf.forward == NULL) {
			return 0;
		}
	}

	return 1;
}

static OUT_BUFFER_ELEMENT *proc_sequence_header(MPEG_VIDEO *in)
{
	int width, height;

	OUT_BUFFER_ELEMENT *r;

	EnterCriticalSection(&(in->lock));
	
	width = in->seq.orig_h_size;
	height = in->seq.orig_v_size;

	r = NULL;
	
	read_sequence_header(&(in->bitstream), &(in->seq));
	if ((width != in->seq.orig_h_size) || (height != in->seq.orig_v_size)) {
		
		r = store_work_frame(in, WORK_FRAME_BWD);
		release_work_frame(in, WORK_FRAME_ALL);

		release_resize_parameter(in->rsz_prm);
		in->rsz_prm = create_force_resize_parameter(&(in->seq), in->width, in->height);
	}
	
	sequence_header_to_bgr_conversion_parameter(&(in->seq), &(in->bgr_prm), &(in->config));
	if(sequence_header_to_yuy2_conversion_parameter(&(in->seq), &(in->ycc_prm), &(in->config))){
		in->yuy2_cc = select_yuy2_convert_function(in->config.simd);
	}else{
		in->yuy2_cc = yuy2_convert_none;
	}
	sequence_header_to_read_picture_header_option(&(in->seq), &(in->pic_opt));
	sequence_header_to_decode_picture_parameter(&(in->seq), in->dec_prm);
	resize_parameter_to_bgr_conversion_parameter(in->rsz_prm, &(in->bgr_prm));

	in->pic.temporal_reference = -1;

	LeaveCriticalSection(&(in->lock));
	
	return r;
}

static OUT_BUFFER_ELEMENT *proc_gop_header(MPEG_VIDEO *in)
{
	OUT_BUFFER_ELEMENT *r;
	
	r = NULL;
	
	vs_erase_bits(&(in->bitstream), 25); /* erase timecode */
	in->closed_gop = vs_get_bits(&(in->bitstream), 1);
	
	if(vs_get_bits(&(in->bitstream), 1)){ /* broken link */
		if ((in->bwd != NULL) && (in->dec_buf.backward != NULL)) {
			r = store_work_frame(in, WORK_FRAME_BWD);
		}
		cancel_work_frame(in, WORK_FRAME_ALL);
	}

	in->pic.temporal_reference = -1;

	return r;
}

static OUT_BUFFER_ELEMENT *proc_picture_data(MPEG_VIDEO *in)
{
	PICTURE_HEADER pic;

	OUT_BUFFER_ELEMENT *r;
	r = NULL;

	memset(&pic, 0, sizeof(pic));
	read_picture_header(&(in->bitstream), &pic, &(in->pic_opt));
	if (pic.temporal_reference == in->pic.temporal_reference) {
		/* broken stream. frame is doubling - skip this frame. */
		return NULL;
	}

	memcpy(&(in->pic), &pic, sizeof(in->pic));
	picture_header_to_decode_picture_parameter(&(in->pic), in->dec_prm);
	setup_current_frame(in);
	picture_header_to_output_parameter(&(in->pic), &(in->cur->prm));
	in->cur->prm.closed_gop = in->closed_gop;

	if (in->pic.picture_coding_type != 3) {
		r = rotate_reference_frame(in);
	}

	if (prepare_pic_decode(in) == 0){
		cancel_work_frame(in, WORK_FRAME_CUR);
		in->dec_prm->mc_parameter.first_field = 0;
		if (check_field_picture(&(in->pic))) {
			skip_2nd_field(&(in->bitstream), &(in->pic_opt), &(in->pic));
		}
		return r;
	}
		
	decode_picture(&(in->bitstream), &(in->dec_buf), in->dec_prm);

	if (check_field_picture(&(in->pic)) != 0) {
		decode_2nd_field(in);
	}

	if(in->dec_buf.forward == in->dec_buf.backward){
		/* reset closed gop error concealment */
		in->dec_buf.forward = NULL;
	}

	if (in->pic.picture_coding_type == 3) {
		r = store_work_frame(in, WORK_FRAME_CUR);
	}

	return r;
}

static OUT_BUFFER_ELEMENT *store_work_frame(MPEG_VIDEO *in, int type)
{
	int i;
	OUT_BUFFER_ELEMENT *r,*s,*d;

	static const int type_table[3] = {
		WORK_FRAME_FWD, 
		WORK_FRAME_CUR,
		WORK_FRAME_BWD,
	};

	r = NULL;
	s = NULL;
	d = NULL;

	EnterCriticalSection(&(in->lock));

	for (i=1;i<3;i++) {
		
		if ((type & type_table[i]) == 0) {
			continue;
		}

		switch (type_table[i]) {
		case WORK_FRAME_CUR:
			s = in->cur;
			s->prm.index = in->cur_index;
			break;
		case WORK_FRAME_BWD:
			s = in->bwd;
			s->prm.index = in->bwd_index;
			break;
		}

		if (s == NULL) {
			continue;
		}

		if (type_table[i] == WORK_FRAME_BWD) {
			if ((s->prm.picture_coding_type == 1) && (s->prm.index != in->current.start_frame)) {
				in->current.frame_count = s->prm.index - in->current.start_frame;
				in->current.start_frame = s->prm.index;
			}
		} else if (type_table[i] == WORK_FRAME_CUR) {
			if ((s->prm.picture_coding_type == 3) && (s->prm.index == in->current.start_frame)) {
				in->current.start_frame += 1;
			}
		}

		in->upsmp_c[s->prm.progressive_frame](&(s->data));

		if (s->prm.repeat_first_field && (in->disp_field_order == s->prm.top_field_first)) {

			if (in->rsz_prm != NULL) {
				d = query_work_frame(&(in->out_buf), in->rsz_prm->l.out_step, in->rsz_prm->l.height);
				resize(&(s->data), &(d->data), in->rsz_prm);
			} else {
				d = query_work_frame(&(in->out_buf), in->seq.h_size, in->seq.v_size);
				copy_frame(&(s->data), &(d->data));
			}

			memcpy(&(d->prm), &(s->prm), sizeof(OUTPUT_PARAMETER));
			set_decoded_frame(&(in->out_buf), d);
			d->ref_count -= 1;

			if (r == NULL) {
				r = d;
			}

			s->prm.index += 1;
			s->prm.top_field_first = !(in->disp_field_order);
			s->prm.repeat_first_field = 0;
			s->prm.picture_coding_type = 3;

			in->bwd_index += 1;
		}

		if (in->rsz_prm != NULL) {
			
			d = query_work_frame(&(in->out_buf), in->rsz_prm->l.out_step, in->rsz_prm->l.height);
			resize(&(s->data), &(d->data), in->rsz_prm);
			memcpy(&(d->prm), &(s->prm), sizeof(OUTPUT_PARAMETER));
			set_decoded_frame(&(in->out_buf), d);
			d->ref_count -= 1;

			if (r == NULL) {
				r = d;
			}
			
		} else {

			set_decoded_frame(&(in->out_buf), s);
			if (type_table[i] == WORK_FRAME_CUR) {
				s->ref_count -= 1;
				in->cur = NULL;
			}

			if (r == NULL) {
				r = s;
			}
		}

		if (type_table[i] == WORK_FRAME_CUR) {
			in->dec_buf.current = NULL;
		}
		
		in->bwd_index += 1;
	}

	if ((r != NULL) && (in->dec_event != NULL)) {
		SetEvent(in->dec_event);
	}

	LeaveCriticalSection(&(in->lock));

	return r;
}

static void cancel_work_frame(MPEG_VIDEO *in, int type)
{
	int i;

	OUT_BUFFER_ELEMENT *w;

	static const int type_table[3] = {
		WORK_FRAME_FWD, 
		WORK_FRAME_CUR,
		WORK_FRAME_BWD,
	};

	w = NULL;
	for (i=0;i<3;i++) {

		if ((type & type_table[i]) == 0) {
			continue;
		}

		switch (type_table[i]) {
		case WORK_FRAME_FWD:
			in->dec_buf.forward = NULL;
			w = in->fwd;
			break;
		case WORK_FRAME_CUR:
			in->dec_buf.current = NULL;
			w = in->cur;
			break;
		case WORK_FRAME_BWD:
			in->dec_buf.backward = NULL;
			w = in->bwd;
			break;
		}

		if (in->rsz_prm != NULL) {
			continue;
		}

		if (w == NULL) {
			continue;
		}

		w->ref_count -= 1;
		if (w->ref_count < 1) {
			back_work_frame(&(in->out_buf), w);
		}			

		switch (type_table[i]) {
		case WORK_FRAME_FWD:
			in->fwd = NULL;
			break;
		case WORK_FRAME_CUR:
			in->cur = NULL;
			break;
		case WORK_FRAME_BWD:
			in->bwd = NULL;
			break;
		}
	}
}

static void release_work_frame(MPEG_VIDEO *in, int type)
{
	int i;

	OUT_BUFFER_ELEMENT *w;

	static const int type_table[3] = {
		WORK_FRAME_FWD, 
		WORK_FRAME_CUR,
		WORK_FRAME_BWD,
	};

	w = NULL;
	for (i=0;i<3;i++) {

		if ((type & type_table[i]) == 0) {
			continue;
		}

		switch (type_table[i]) {
		case WORK_FRAME_FWD:
			in->dec_buf.forward = NULL;
			w = in->fwd;
			break;
		case WORK_FRAME_CUR:
			in->dec_buf.current = NULL;
			w = in->cur;
			break;
		case WORK_FRAME_BWD:
			in->dec_buf.backward = NULL;
			w = in->bwd;
			break;
		}

		if (w == NULL) {
			continue;
		}

		w->ref_count -= 1;
		if (w->ref_count < 1) {
			back_work_frame(&(in->out_buf), w);
		}			

		switch (type_table[i]) {
		case WORK_FRAME_FWD:
			in->fwd = NULL;
			break;
		case WORK_FRAME_CUR:
			in->cur = NULL;
			break;
		case WORK_FRAME_BWD:
			in->bwd = NULL;
			break;
		}
	}
}

static void setup_current_frame(MPEG_VIDEO *in)
{
	while ((in->out_buf.dec.count > 4) && (in->thrd_stat == THREAD_STAT_RUNNING)) {
		WaitForSingleObject(in->req_event, 100);
	}
	
	if (in->cur == NULL) {
		in->cur = query_work_frame(&(in->out_buf), in->seq.h_size, in->seq.v_size);
		goto LAST;
	}

	if ((in->cur->data.width  != in->seq.h_size) ||
	    (in->cur->data.height != in->seq.v_size)) {
		release_work_frame(in, WORK_FRAME_CUR);
		in->cur = query_work_frame(&(in->out_buf), in->seq.h_size, in->seq.v_size);
		goto LAST;
	}

LAST:
	in->cur_index = in->bwd_index;
	in->dec_buf.current = &(in->cur->data);
}

static OUT_BUFFER_ELEMENT *rotate_reference_frame(MPEG_VIDEO *in)
{
	OUT_BUFFER_ELEMENT *r,*w;
	
	r = NULL;

	if (in->fwd != NULL) {
		cancel_work_frame(in, WORK_FRAME_FWD);
	}
	
	in->fwd_index = in->bwd_index;
	if (in->dec_buf.backward != NULL) {
		r = store_work_frame(in, WORK_FRAME_BWD);
	}
	
	w = in->fwd;
	in->fwd = in->bwd;
	in->bwd = in->cur;
	in->cur = w;

	in->dec_buf.forward = in->dec_buf.backward;
	in->dec_buf.backward = in->dec_buf.current;

	if (in->closed_gop == 1) {
		in->closed_gop = 2;
	} else {
		in->closed_gop = 0;
	}

	return r;
}

static int is_seek_required(MPEG_VIDEO *p, __int64 frame)
{
	__int64 n,m;
	
	if(p->dec_buf.backward){
		if(p->bwd_index == frame){
			return 0;
		}
	}
	
	n = p->current.start_frame + p->current.frame_count;
	m = p->bwd_index + 10;
	if(n<m){
		n = m;
	}
	
	if( (p->bwd_index > 0) && (frame >= p->bwd_index) && (frame < n) ){
		return 0;
	}

	return 1;
}

static void sequence_header_to_decode_picture_parameter(SEQUENCE_HEADER *in, DECODE_PICTURE_PARAMETER *out)
{
	sequence_header_to_read_slice_header_option(in, &(out->slice_option));
	sequence_header_to_read_macroblock_option(in, &(out->macroblock_option));
	sequence_header_to_read_block_option(in, &(out->block_option));
	sequence_header_to_mc_parameter(in, &(out->mc_parameter));
}

static void picture_header_to_decode_picture_parameter(PICTURE_HEADER *in, DECODE_PICTURE_PARAMETER *out)
{
	picture_header_to_read_macroblock_option(in, &(out->macroblock_option));
	picture_header_to_read_block_option(in, &(out->block_option));
	picture_header_to_mc_parameter(in, &(out->mc_parameter));
}

static void decode_2nd_field(MPEG_VIDEO *p)
{
	int code;
	__int64 offset;

	PICTURE_HEADER pic;
	
	while(vs_next_start_code(&(p->bitstream))){
		code = vs_read_bits(&(p->bitstream), 32);
		if(code == 0x100){
			offset = video_stream_tell(&(p->bitstream));
			vs_erase_bits(&(p->bitstream), 32);
			
			read_picture_header(&(p->bitstream), &pic, &(p->pic_opt));

			if ((pic.has_picture_coding_extension == 0) || (pic.pc.picture_structure == 3)) {
				/* broken stream */
				/* 2nd field is droped */
				copy_2nd_field(p->dec_buf.current, p->pic.pc.picture_structure);
				video_stream_seek(&(p->bitstream), offset, SEEK_SET);
				return;
			}

			if (pic.pc.picture_structure == p->pic.pc.picture_structure) {
				/* broken stream */
				if (pic.temporal_reference == p->pic.temporal_reference) {
					/* 1st field is doubling */
					continue;
				}
				/* 2nd field is droped */
				copy_2nd_field(p->dec_buf.current, p->pic.pc.picture_structure);
				video_stream_seek(&(p->bitstream), offset, SEEK_SET);
				return;
			}
			
			picture_header_to_decode_picture_parameter(&pic, p->dec_prm);
			decode_picture(&(p->bitstream), &(p->dec_buf), p->dec_prm);

			return;
		} else {
			continue;
		}
	}
}

static int is_registered_suffix(char *filepath)
{
#if 0
	int i;
	
	static char *registered_suffix[] = {
		".mpeg",
		".mpg",
		".m2p",
		".mp2",
		".vob",
		".vro",
		".m2v",
		".m1v",
		".mpv",
		".ves",
		".m2t",
		".ssg",
		".ts",
		".bs",
		".m2ts",
		"", /* sentinel */
	};

	i = 0;
	while(registered_suffix[i][0]){
		if(check_suffix(filepath, registered_suffix[i])){
			return 1;
		}
		i += 1;
	}

	return 0;
#else
	return check_suffix(filepath, ".tvtv");
#endif
}

static void verify_sequence_size(MPEG_VIDEO *in)
{
	int i;
	__int64 offset;
	__int64 save_offset;

	if(in->total < (2*8)) {
		// no need to verify
		return;
	}

	save_offset = video_stream_tell(&(in->bitstream));

	for(i=0;i<8;i++){
		offset = i * (in->total/8);
		verify_sequence_header(in, offset);
	}

	video_stream_seek(&(in->bitstream), save_offset, SEEK_SET);
}

static void verify_sequence_header(MPEG_VIDEO *in, __int64 frame)
{
	int code;
	__int64 limit;
	SEQUENCE_HEADER work;
	GOP gop;

	gop = in->fg.func(in->fg.arg1, frame);
	
	limit = get_filecheck_limit();
	limit += gop.offset;

	video_stream_seek(&(in->bitstream), gop.offset, SEEK_SET);

	code = 0;
	while(vs_next_start_code(&(in->bitstream))){
		code = vs_get_bits(&(in->bitstream), 32);
		if(code == 0x1B3){
			break;
		}else if(limit < video_stream_tell(&(in->bitstream))){
			/* limit over */
			break;
		}
	}

	if(code != 0x1B3){
		/* failed to find sequence_header */
		return; 
	}

	if(!read_sequence_header(&(in->bitstream), &work)){
		/* failed on read_sequence_header() */
		return;
	}

	if ( (in->seq.h_size < work.h_size) ||
	     (in->seq.v_size < work.v_size) ){
		/* find more large frame */
		memcpy(&(in->seq), &work, sizeof(SEQUENCE_HEADER));
	}
}

static void setup_chroma_upsampling_function(MPEG_VIDEO *p, int chroma_format, int simd)
{
	if(chroma_format == 1){
		if(simd & M2V_CONFIG_USE_SSE2){
			p->upsmp_c[0] = upsample_chroma_420i_sse2;
			p->upsmp_c[1] = upsample_chroma_420p_sse2;
		}else if(simd & M2V_CONFIG_USE_MMX){
			p->upsmp_c[0] = upsample_chroma_420i_mmx;
			p->upsmp_c[1] = upsample_chroma_420p_mmx;
		}else{
			p->upsmp_c[0] = upsample_chroma_420i;
			p->upsmp_c[1] = upsample_chroma_420p;
		}
	}else{
		p->upsmp_c[0] = upsample_chroma_none;
		p->upsmp_c[1] = upsample_chroma_none;
	}
}

static void setup_convert_function(MPEG_VIDEO *p, int chroma_format, int simd)
{
	if(chroma_format == 3){
		p->to_bgr = yuv444_to_bgr;
		p->to_yuy2 = yuv444_to_yuy2;
	}else{
		if(simd & M2V_CONFIG_USE_SSE2){
			p->to_bgr = yuv422_to_bgr_sse2;
			p->to_yuy2 = yuv422_to_yuy2_sse2;
		}else if(simd & M2V_CONFIG_USE_MMX){
			p->to_bgr = yuv422_to_bgr_mmx;
			p->to_yuy2 = yuv422_to_yuy2_mmx;
		}else{
			p->to_bgr = yuv422_to_bgr;
			p->to_yuy2 = yuv422_to_yuy2;
		}
	}
}

static void setup_qw_function(DECODE_PICTURE_PARAMETER *p, int simd)
{
	if(simd & M2V_CONFIG_USE_SSE2){
		p->block_option.setup_qw = setup_qw_sse2;
	}else if(simd & M2V_CONFIG_USE_MMX){
		p->block_option.setup_qw = setup_qw_mmx;
	}else{
		p->block_option.setup_qw = setup_qw_nosimd;
	}
}

static void setup_idct_function(DECODE_PICTURE_PARAMETER *p, M2V_CONFIG *prm)
{
	if(prm->idct_type == M2V_CONFIG_IDCT_REFERENCE){
		if(prm->simd & M2V_CONFIG_USE_SSE){
			p->idct_func = idct_reference_sse;
		}else{
			p->idct_func = idct_reference;
		}
	}else if (prm->idct_type == M2V_CONFIG_IDCT_LLM_INT){
		if(prm->simd & M2V_CONFIG_USE_MMX){
			p->idct_func = idct_llm_mmx;
		}else{
			p->idct_func = idct_llm_int;
		}
	}else{
		if(prm->simd & M2V_CONFIG_USE_SSE2){
			p->idct_func = idct_ap922_sse2;
		}else if(prm->simd & M2V_CONFIG_USE_SSE){
			p->idct_func = idct_ap922_sse;
		}else if(prm->simd & M2V_CONFIG_USE_MMX){
			p->idct_func = idct_ap922_mmx;
		}else{
			p->idct_func = idct_ap922_int;
		}
	}
}

static void setup_mc_function(DECODE_PICTURE_PARAMETER *p, M2V_CONFIG *prm)
{
	if(prm->simd & M2V_CONFIG_USE_SSE2){
		p->mc_parameter.prediction_func = prediction_sse2;
	}else if(prm->simd & M2V_CONFIG_USE_SSE){
		p->mc_parameter.prediction_func = prediction_sse;
	}else if(prm->simd & M2V_CONFIG_USE_MMX){
		p->mc_parameter.prediction_func = prediction_mmx;
	}else{
		p->mc_parameter.prediction_func = prediction;
	}
}

static void setup_add_block_function(DECODE_PICTURE_PARAMETER *p, M2V_CONFIG *prm)
{
	if(prm->simd & M2V_CONFIG_USE_MMX){
		p->add_block_func = add_block_data_to_frame_mmx;
	}else{
		p->add_block_func = add_block_data_to_frame;
	}
}

static void reset_working_frames(MPEG_VIDEO *p)
{
	memset(&(p->dec_buf), 0, sizeof(MC_BUFFER));

	release_work_frame(p, WORK_FRAME_ALL);

	p->fwd_index = p->current.start_frame-1;
	p->bwd_index = p->current.start_frame;
	p->pic.temporal_reference = -1;
}

static void setup_m2v_config(M2V_CONFIG *p)
{
	p->simd = get_simd_mode();
	p->bt601 = get_color_conversion_type();
	p->yuy2 = get_yuy2_mode();
	p->aspect_ratio = get_resize_mode();
	p->field_mode = get_field_mode();
	p->idct_type = get_idct_type();
	p->color_matrix = get_color_matrix();
	p->gl = get_gl_mode();
}

static void setup_field_order(MPEG_VIDEO *p, M2V_CONFIG *prm)
{
	switch(prm->field_mode){
	case M2V_CONFIG_FRAME_KEEP_ORIGINAL:
		p->disp_field_order = p->orig_field_order;
		break;
	case M2V_CONFIG_FRAME_TOP_FIRST:
		p->disp_field_order = TOP_FIELD_FIRST;
		break;
	case M2V_CONFIG_FRAME_BOTTOM_FIRST:
		p->disp_field_order = BOTTOM_FIELD_FIRST;
		break;
	default:
		p->disp_field_order = p->orig_field_order;
	}

	if(p->disp_field_order != p->orig_field_order){
		p->total -= 1;
	}
}

static void resize_parameter_to_bgr_conversion_parameter(RESIZE_PARAMETER *in, BGR_CONVERSION_PARAMETER *out)
{
	if(in == NULL){
		return;
	}

	out->prm.width = in->l.width;
	out->prm.height = in->l.height;

	out->prm.in_step = in->l.out_step;

	out->prm.c_offset = in->c.out_offset;
}

static YUY2_CONVERT select_yuy2_convert_function(int simd)
{
	YUY2_CONVERT r;

	r = yuy2_convert;
	
	if(simd & M2V_CONFIG_USE_SSE2){
		r = yuy2_convert_sse2;
	}else if(simd & M2V_CONFIG_USE_SSE){
		r = yuy2_convert_mmx;
	}else if(simd & M2V_CONFIG_USE_MMX){
		r = yuy2_convert_mmx;
	}
	
	return r;
}

