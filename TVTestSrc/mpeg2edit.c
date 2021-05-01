/*******************************************************************
                    MPEG-2 stream edit module
 *******************************************************************/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>

#include "bitstream.h"
#include "vfapi.h"

#define MPEG2EDIT_C
#include "mpeg2edit.h"

typedef struct {
	unsigned char *data;
	int length;

	void *next;
	void *prev;
} LIST_ELEMENT;

typedef struct {
	int length;
	LIST_ELEMENT *head;
} ES_SEQUENCE_HEADER;

typedef struct {
	int length;
	LIST_ELEMENT *head;
} ES_GOP_HEADER;

typedef struct {
	int length;
	LIST_ELEMENT *head;
} ES_PICTURE;

typedef struct {
	ES_PICTURE fst;
	ES_PICTURE snd;
} ES_FRAME;

typedef struct {
	LIST_ELEMENT *pack_header;
	LIST_ELEMENT *pes_packet;

	__int64 scr_length;
} PS_PACK;

typedef struct {
	PS_PACK *data;

	void *prev;
	void *next;
} PS_PACK_LIST_ELEMENT;

typedef struct {

	__int64 in;
	__int64 out;

	__int64 input_field;
	__int64 output_field;

	int stream_id;
	
	int offset;
	
	int sequence;
	int picture;

	int output;

	int level;
	int step;

	int temporal_reference;
	
	int padding;

	int rate;
	int scale;
	int fps;

	int closed_gop;
	int broken_link;
	
} PS_PROC_VIDEO_OPTION;

static const int PS_WASTE_PACK = 0;
static const int PS_OUTPUT_PACK = 1;
static const int PS_HEADER_PACK = 2;
static const int PS_HEADER_CHAIN_PACK = 4;

static const int PS_LEVEL_SEQUENCE = 0;
static const int PS_LEVEL_GOP = 1;
static const int PS_LEVEL_PICTURE = 2;
static const int PS_LEVEL_FIELD = 3;

static const int PS_STEP_START = 0;
static const int PS_STEP_SEEK = 1;
static const int PS_STEP_OUTPUT = 2;
static const int PS_STEP_END = 3;

typedef struct {
	unsigned char *data;
	int length;
} BLOCK;

typedef struct {
	__int64 head[256];
	__int64 tail[256];
	int count;
} STREAM_ID_LIST;

/* function prototypes */
int open_mpeg2edit(const char *path, MPEG2EDIT *mpeg2edit);

static void dummy_edit(void *mpeg2edit, char *out_path, __int64 in, __int64 out);
static void dummy_close(void *mpeg2edit);
static HRESULT _stdcall dummy_callback(char *out_path, DWORD percent);

static LIST_ELEMENT *new_list_element(LIST_ELEMENT *pos, int unit_size);
static void fwrite_list(LIST_ELEMENT *head, FILE *out);
static void clear_list(LIST_ELEMENT *head);
static LIST_ELEMENT *copy_list(LIST_ELEMENT *in);
static void padding_list(LIST_ELEMENT *head, int data);
static unsigned char get_byte_list(LIST_ELEMENT *head, int offset);
static void set_byte_list(LIST_ELEMENT *head, int offset, unsigned char byte);
static int find_list(LIST_ELEMENT *head, int offset, unsigned char *pattern, int length);
static int count_list_size(LIST_ELEMENT *head);

static BLOCK *new_block(unsigned char *data, int length);
static void delete_block(BLOCK *block);
static int find_block(BLOCK *current, BLOCK *next, int offset, BLOCK *pattern);
static unsigned char get_byte_block(BLOCK *current, BLOCK *next, int offset);
static void set_byte_block(BLOCK *current, BLOCK *next, int offset, unsigned char data);

typedef int (* judge_read_list_end)(int code);
static LIST_ELEMENT *read_list(BITSTREAM *in, int unit_size, judge_read_list_end func); 

static int subtract_temporal_reference(int current, int previous);

static int check_ts(BITSTREAM *in);

static int es_open(const char *path, MPEG2EDIT *mpeg2edit);
static void es_edit(void *mpeg2edit, char *out_path, __int64 in, __int64 out);
static void es_close(void *mpeg2edit);
static int es_read_sequence_header(BITSTREAM *in, ES_SEQUENCE_HEADER *out);
static int es_read_gop_header(BITSTREAM *in, ES_GOP_HEADER *out);
static int es_read_picture(BITSTREAM *in, ES_PICTURE *out);
static int es_read_frame(BITSTREAM *in, ES_FRAME *out);
static int es_judge_read_sequence_header_end(int code);
static int es_judge_read_gop_header_end(int code);
static int es_judge_read_picture_end(int code);
static void es_write_sequence_header(ES_SEQUENCE_HEADER *in, FILE *out);
static void es_write_gop_header(ES_GOP_HEADER *in, FILE *out);
static void es_write_picture(ES_PICTURE *in, FILE *out);
static void es_write_frame(ES_FRAME *in, FILE *out);
static void es_write_sequence_end_code(FILE *out);
static void es_clear_sequence_header(ES_SEQUENCE_HEADER *seq);
static void es_clear_gop_header(ES_GOP_HEADER *gop);
static void es_clear_picture(ES_PICTURE *pic);
static void es_clear_frame(ES_FRAME *frm);
static void es_padding_picture(ES_PICTURE *pic);
static void es_padding_frame(ES_FRAME *frm);
static int es_get_fps(ES_SEQUENCE_HEADER *seq);
static void es_set_timecode(ES_GOP_HEADER *gop, __int64 frame, int fps);
static int es_get_closed_gop(ES_GOP_HEADER *gop);
static int es_get_broken_link(ES_GOP_HEADER *gop);
static void es_set_broken_link(ES_GOP_HEADER *gop, int broken_link);
static void es_set_closed_gop(ES_GOP_HEADER *gop);
static int  es_get_picture_temporal_reference(ES_PICTURE *pic);
static void es_set_picture_temporal_reference(ES_PICTURE *pic, int temporal_reference);
static int es_get_picture_coding_type_picture(ES_PICTURE *pic);
static int es_get_picture_field_count(ES_PICTURE *pic);
static int es_get_temporal_reference(ES_FRAME *frm);
static void es_set_temporal_reference(ES_FRAME *frm, int temporal_reference);
static int es_get_coding_type(ES_FRAME *frm);
static int es_get_field_count(ES_FRAME *frm);

static int ps_open(const char *path, MPEG2EDIT *mpeg2edit);
static void ps_edit(void *mpeg2edit, char *out_path, __int64 in, __int64 out);
static void ps_close(void *mpeg2edit);
static void ps_write_program_end_code(FILE *out);
static int ps_read_pack(BITSTREAM *in, PS_PACK *out);
static void ps_write_pack(PS_PACK *in, FILE *out, STREAM_ID_LIST *audio_stream);
static void ps_clear_pack(PS_PACK *pack);
static void ps_copy_system_header_pack(PS_PACK *in, PS_PACK *out);
static int ps_find_system_header(PS_PACK *pack);
static int ps_get_video_stream_id(PS_PACK *pack);
static int ps_find_stream(PS_PACK *pack, int stream_id);
static __int64 ps_get_system_clock_reference(PS_PACK *pack);
static void ps_set_system_clock_reference(PS_PACK *pack, __int64 scr);
static __int64 ps_get_pack_scr_length(PS_PACK *pack);
static void ps_set_pack_scr_length(PS_PACK *pack, __int64 scr_length);
static int ps_proc_video(PS_PACK *current, PS_PACK *next, PS_PROC_VIDEO_OPTION *opt);
static int ps_judge_read_pack_header_end(int code);
static LIST_ELEMENT *ps_read_pes_packet(BITSTREAM *in);
static void ps_init_proc_video_option(PS_PROC_VIDEO_OPTION *opt, __int64 in, __int64 out);
static void ps_set_stream_id_proc_video_option(PS_PROC_VIDEO_OPTION *opt, int stream_id);
static int ps_get_current_percent(PS_PROC_VIDEO_OPTION *opt);
static PS_PACK_LIST_ELEMENT *ps_add_pack_list(PS_PACK_LIST_ELEMENT *pos, PS_PACK *data);
static void ps_write_pack_list(PS_PACK_LIST_ELEMENT *head, FILE *out, STREAM_ID_LIST *audio_list);
static void ps_clear_pack_list(PS_PACK_LIST_ELEMENT *head);
static int ps_check_pack_chain(PS_PACK_LIST_ELEMENT *tail, PS_PACK *next);
static __int64 ps_get_pack_list_scr_length(PS_PACK_LIST_ELEMENT *head);
static void ps_set_pack_list_scr(PS_PACK_LIST_ELEMENT *head, __int64 scr);
static void ps_set_clock_reference_pes_packet(LIST_ELEMENT *pes_packet, __int64 diff);
static BLOCK *ps_join_stream(PS_PACK *pack, int stream_id);
static void ps_update_stream(PS_PACK *pack, int stream_id, BLOCK *data);
static BLOCK *ps_get_pes_packet_data(LIST_ELEMENT *pes_packet);
static void ps_set_pes_packet_data(LIST_ELEMENT *pes_packet, BLOCK *data);
static int ps_get_pes_packet_data_length(LIST_ELEMENT *pes_packet);
static int ps_get_pes_packet_header_length(LIST_ELEMENT *pes_packet);
static int ps_proc_video_picture(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt);
static int ps_proc_video_sequence(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt);
static int ps_proc_video_end(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt);
static int ps_proc_video_gop(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt);
static int ps_proc_video_other(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt);
static int ps_proc_video_extension(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt);
static int ps_check_fill_zero_video(PS_PROC_VIDEO_OPTION *opt);
static void ps_fill_zero_video(BLOCK *b1, BLOCK *b2, int from, int to);
static void ps_initialize_video_packet(PS_PACK *pack, int stream_id);
static void ps_terminate_video_packet(PS_PACK *pack, int stream_id);
static void ps_init_stream_id_list(STREAM_ID_LIST *list);
static void ps_add_stream_id_list(int id, __int64 offset, STREAM_ID_LIST *list);
static void ps_remove_stream_id_list(int id, STREAM_ID_LIST *list);
static int ps_check_stream_id_list(int id, STREAM_ID_LIST *list);
static int ps_check_empty_stream_id_list(STREAM_ID_LIST *list);
static __int64 ps_get_head_offset_stream_id_list(int id, STREAM_ID_LIST *list);
static __int64 ps_get_tail_offset_stream_id_list(int id, STREAM_ID_LIST *list);
static void ps_check_audio_stream(PS_PACK *pack, __int64 offset, STREAM_ID_LIST *audio_stream);
static void ps_trim_audio_packet(const char *path, STREAM_ID_LIST *audio_stream);
static void ps_trim_head_audio_packet(int fd, int stream_id, __int64 offset);
static void ps_trim_tail_audio_packet(int fd, int stream_id, __int64 offset);

static unsigned char *as_find_sync(unsigned char *buf, int size);
static int as_get_frame_size(unsigned char *header, int size);

/*-----------------------------------------------------------------*/
int open_mpeg2edit(const char *path, MPEG2EDIT *mpeg2edit)
{
	int n;
	int code;
	BITSTREAM *bs;

	int system;

	mpeg2edit->edit = dummy_edit;
	mpeg2edit->callback = dummy_callback;
	mpeg2edit->close = dummy_close;
	
	bs = bs_open(path);
	if(bs == NULL){
		return 0;
	}

	if(check_ts(bs)){
		return 0;
	}

	n = 0;
	system = 0;
	while(bs_next_packet_prefix(bs)){
		code = bs_read_bits(bs, 32);
		if(code >= 0x1ba){
			system += 1;
		}

		n += 1;

		if(n == 8){
			break;
		}
	}

	bs_close(bs);

	if(n < 8){
		return 0;
	}

	if(system){
		return ps_open(path, mpeg2edit);
	}
	
	return es_open(path, mpeg2edit);
}

/*-----------------------------------------------------------------*/
static void dummy_edit(void *mpeg2edit, char *out_path, __int64 in, __int64 out)
{
	return;
}

static HRESULT _stdcall dummy_callback(char *path, DWORD percent)
{
	return VF_OK; /* do nothing */
}

static void dummy_close(void *mpeg2edit)
{
	MPEG2EDIT *p;

	p = (MPEG2EDIT *)mpeg2edit;

	p->stream = NULL;
	p->edit = dummy_edit;
	p->close = dummy_close;
}

/*-----------------------------------------------------------------*/
static LIST_ELEMENT *new_list_element(LIST_ELEMENT *pos, int unit_size)
{
	LIST_ELEMENT *element;

	element = (LIST_ELEMENT *)calloc(1, sizeof(LIST_ELEMENT));
	if(element == NULL){
		return NULL;
	}
	
	element->data = (unsigned char *)malloc(unit_size);
	if(element->data == NULL){
		free(element);
		return NULL;
	}
	
	element->length = 0;

	element->prev = pos;

	if(pos != NULL){
		element->next = pos->next;
		pos->next = element;
	}

	return element;
}

static void fwrite_list(LIST_ELEMENT *head, FILE *out)
{
	if(out == NULL){
		return;
	}

	while(head){
		fwrite(head->data, 1, head->length, out);
		head = (LIST_ELEMENT *)head->next;
	}
}

static void clear_list(LIST_ELEMENT *head)
{
	LIST_ELEMENT *current;
	LIST_ELEMENT *next;

	current = head;

	while(current){
		free(current->data);
		next = (LIST_ELEMENT *)current->next;
		free(current);
		current = next;
	}
}

static LIST_ELEMENT *copy_list(LIST_ELEMENT *head)
{
	LIST_ELEMENT *r;
	LIST_ELEMENT *current;

	if(head == NULL){
		return NULL;
	}

	current = new_list_element(NULL, head->length);
	r = current;

	memcpy(current->data, head->data, head->length);
	current->length = head->length;
	
	head = (LIST_ELEMENT *)head->next;
	
	while(head){
		current = new_list_element(current, head->length);
		memcpy(current->data, head->data, head->length);
		current->length = head->length;
		head = (LIST_ELEMENT *)head->next;
	}

	return r;
}

static void padding_list(LIST_ELEMENT *head, int data)
{
	while(head){
		memset(head->data, data, head->length);
		head = (LIST_ELEMENT *)head->next;
	}
}

static unsigned char get_byte_list(LIST_ELEMENT *head, int offset)
{
	int n;

	n = 0;
	
	while(head){
		if(offset - n < head->length){
			return head->data[offset-n];
		}
		n += head->length;
		head = (LIST_ELEMENT *)head->next;
	}

	return 0;
}

static void set_byte_list(LIST_ELEMENT *head, int offset, unsigned char byte)
{
	int n;

	n = 0;

	while(head){
		if(offset - n < head->length){
			head->data[offset-n] = byte;
			return;
		}
		n += head->length;
		head = (LIST_ELEMENT *)head->next;
	}
}

static int find_list(LIST_ELEMENT *head, int offset, unsigned char *pattern, int length)
{
	int r;
	int i,n;
	
	LIST_ELEMENT *next;

	next = (LIST_ELEMENT *)head->next;

	n = 0;
	r = offset;
	while(head){
		if(r-n < head->length){
			for(i=0;i<length;i++){
				if(r-n+i < head->length){
					if(head->data[r-n+i] == pattern[i]){
						continue;
					}else{
						break;
					}
				}else if(next == NULL){
					return -1;
				}else{
					if(next->data[r-n-head->length+i] == pattern[i]){
						continue;
					}else{
						break;
					}
				}
			}
			if(i == length){
				return r;
			}
			r += 1;
		}else{
			n += head->length;
			head = next;
			if(next != NULL){
				next = (LIST_ELEMENT *)head->next;
			}
		}
	}

	return -1;
}

static int count_list_size(LIST_ELEMENT *head)
{
	int r;

	r = 0;
	
	while(head){
		r += head->length;
		head = (LIST_ELEMENT *)head->next;
	}

	return r;
}

/*-----------------------------------------------------------------*/
static LIST_ELEMENT *read_list(BITSTREAM *in, int unit_size, judge_read_list_end func)
{
	int n,code;
	
	LIST_ELEMENT *r;
	LIST_ELEMENT *current;

	current = new_list_element(NULL, unit_size);
	if(current == NULL){
		return NULL;
	}

	r = current;
	code = bs_read_bits(in, 32);

	do{
		if( (current->length+4) >= unit_size){
			current = new_list_element(current, unit_size);
			if(current == NULL){
				return 0;
			}
		}

		if( (code >> 8) == 1 ){
			n = bs_read(in, current->data+current->length, 4);
			current->length += n;
		}
		n = bs_read_next_packet_prefix(in, current->data+current->length, unit_size-current->length);
		current->length += n;
		
		code = bs_read_bits(in, 32);
		
	}while(n && (!func(code)));

	return r;
}

/*-----------------------------------------------------------------*/
static BLOCK *new_block(unsigned char *data, int length)
{
	BLOCK *r;

	r = (BLOCK *)malloc(sizeof(BLOCK));
	if(r == NULL){
		return NULL;
	}

	r->data = (unsigned char *)calloc(length, 1);
	if(r->data == NULL){
		free(r);
		return NULL;
	}

	r->length = length;
	memcpy(r->data, data, length);

	return r;
}

static void delete_block(BLOCK *block)
{
	if(block){
		if(block->data){
			free(block->data);
			block->data = 0;
		}
		block->length = 0;
		free(block);
	}
}

static int find_block(BLOCK *current, BLOCK *next, int offset, BLOCK *pattern)
{
	int r;
	int i;

	r = offset;

	while(r < current->length){
		for(i=0;i<pattern->length;i++){
			if(r+i<current->length){
				if(current->data[r+i] == pattern->data[i]){
					continue;
				}else{
					break;
				}
			}else if(next != NULL){
				if(next->data[r-current->length+i] == pattern->data[i]){
					continue;
				}else{
					break;
				}
			}else{
				return -1;
			}
		}
		if(i == pattern->length){
			return r;
		}
		r += 1;
	}

	return -1;
}

static unsigned char get_byte_block(BLOCK *current, BLOCK *next, int offset)
{
	if(current == NULL){
		return 0;
	}

	if(offset < current->length){
		return current->data[offset];
	}else if( (next != NULL) && (offset-current->length < next->length) ){
		return next->data[offset-current->length];
	}

	return 0;
}

static void set_byte_block(BLOCK *current, BLOCK *next, int offset, unsigned char data)
{
	if(current == NULL){
		return;
	}

	if(offset < current->length){
		current->data[offset] = data;
	}else if( (next != NULL) && (offset-current->length < next->length) ){
		next->data[offset-current->length] = data;
	}
}

/*-----------------------------------------------------------------*/
static int subtract_temporal_reference(int current, int previous)
{
	if( (previous & 0x0200) && ((current & 0x03fe) == 0) ){
		current |= 0x0400;
	}

	return current - previous;
}

/*-----------------------------------------------------------------*/
static int check_ts(BITSTREAM *in)
{
	int i,n;
	int o,p,d;
	int count[376];
	int max, total;
	unsigned char buffer[460224];

	n = bs_read(in, buffer, sizeof(buffer));
	bs_seek(in, SEEK_SET, 0);

	o = 0;
	for(i=0;i<n;i++){
		if(buffer[i] == 0x47){
			o = i;
			break;
		}
	}

	if(o > 188){
		return 0;
	}
	p = o;
	total = 0;
	memset(count, 0, sizeof(count));
	for(;i<n;i++){
		if(buffer[i] == 0x47){
			d = i-p;
			if(d >= sizeof(count)/sizeof(int)){
				return 0;
			}
			count[d] += 1;
			total += 1;
			p = i;
		}
	}

	max = 0;
	for(i=0;i<sizeof(count)/sizeof(int);i++){
		if(max < count[i]){
			max = count[i];
		}
	}

	if(max > total/2){
		return 1;
	}

	return 0;
}

/*-----------------------------------------------------------------*/
static int es_open(const char *path, MPEG2EDIT *mpeg2edit)
{
	BITSTREAM *bs;
	
	bs = bs_open(path);
	if(bs == NULL){
		return 0;
	}

	bs_seek(bs, 0, SEEK_SET);

	mpeg2edit->stream = bs;
	mpeg2edit->edit = es_edit;
	mpeg2edit->close = es_close;
	
	return 1;
}

static void es_edit(void *mpeg2edit, char *out_path, __int64 in, __int64 out)
{
	int current_temporal_reference;
	int previous_temporal_reference;

	int current_percent;

	__int64 input_field_count;
	__int64 output_field_count;

	int closed_gop;
	int broken_link;
	int fps;

	__int64 n;

	ES_SEQUENCE_HEADER seq;
	ES_GOP_HEADER gop;
	ES_FRAME frm;

	BITSTREAM *bs;

	FILE *file;

	bs = (BITSTREAM *)((MPEG2EDIT *)mpeg2edit)->stream;

	bs_seek(bs, 0, SEEK_SET);

	current_temporal_reference = 0;
	previous_temporal_reference = 0;

	input_field_count = 0;

	memset(&seq, 0, sizeof(seq));
	memset(&gop, 0, sizeof(gop));
	memset(&frm, 0, sizeof(frm));

	closed_gop = 0;
	broken_link = 0;
	
	/* 1st step - search stream head I picture */
	while(bs_next_packet_prefix(bs)){
		switch(bs_read_bits(bs, 32)){
		case 0x000001b3:
			es_clear_sequence_header(&seq);
			es_read_sequence_header(bs, &seq);
			break;
		case 0x000001b8:
			es_clear_gop_header(&gop);
			es_read_gop_header(bs, &gop);
			current_temporal_reference = 0;
			previous_temporal_reference = 0;
			closed_gop = es_get_closed_gop(&gop);
			break;
		case 0x00000100:
			es_clear_frame(&frm);
			es_read_frame(bs, &frm);
			if(es_get_coding_type(&frm) == 1){
				if(closed_gop == 0){
					broken_link = 1;
				}
				if(seq.length){
					goto ES_EDIT_2ND_STEP;
				}
			}
			break;
		default:
			bs_erase_bits(bs, 32);
		}
	}

	es_clear_sequence_header(&seq);
	es_clear_gop_header(&gop);
	es_clear_frame(&frm);
	
	return;

ES_EDIT_2ND_STEP: /* search in point */

	if( in == 0 ){
		goto ES_EDIT_3RD_STEP;
	}

	previous_temporal_reference = es_get_temporal_reference(&frm);
	if( closed_gop && (in <= subtract_temporal_reference(previous_temporal_reference, 0)) ){
		goto ES_EDIT_3RD_STEP;
	}
	input_field_count += es_get_field_count(&frm);

	while(bs_next_packet_prefix(bs)){
		
		current_percent = (int)(input_field_count * 50 / out);
		if(((MPEG2EDIT *)mpeg2edit)->callback(out_path, current_percent) != VF_OK){
			es_clear_sequence_header(&seq);
			es_clear_gop_header(&gop);
			es_clear_frame(&frm);
			
			return;
		}
		
		switch(bs_read_bits(bs, 32)){
		case 0x000001b3:
			es_clear_sequence_header(&seq);
			es_read_sequence_header(bs, &seq);
			break;
		case 0x000001b8:
			es_clear_gop_header(&gop);
			es_read_gop_header(bs, &gop);
			closed_gop = es_get_closed_gop(&gop);
			broken_link = es_get_broken_link(&gop);
			current_temporal_reference = 0;
			previous_temporal_reference = 0;
			break;
		case 0x00000100:
			es_clear_frame(&frm);
			es_read_frame(bs, &frm);
			switch(es_get_coding_type(&frm)){
			case 1:
				if(broken_link == 1){
					broken_link = 2;
					current_temporal_reference = 0;
				}else{
					broken_link = 0;
					current_temporal_reference = es_get_temporal_reference(&frm);
				}
				if(closed_gop == 1){
					closed_gop = 2;
				}else{
					closed_gop = 0;
				}
				n = (input_field_count+1)/2;
				n += subtract_temporal_reference(current_temporal_reference, previous_temporal_reference);
				if( n >= in ){
					goto ES_EDIT_3RD_STEP;
				}
				previous_temporal_reference = es_get_temporal_reference(&frm);
				input_field_count += es_get_field_count(&frm);
				break;
			case 2:
				broken_link = 0;
				closed_gop = 0;
				previous_temporal_reference = es_get_temporal_reference(&frm);
				input_field_count += es_get_field_count(&frm);
				break;
			case 3:
				if(broken_link == 0){
					input_field_count += es_get_field_count(&frm);
				}
			}
			break;
		default:
			bs_erase_bits(bs, 32);
		}
	}

	es_clear_sequence_header(&seq);
	es_clear_gop_header(&gop);
	es_clear_frame(&frm);
	
	return;

ES_EDIT_3RD_STEP: /* output data to file */

	output_field_count = 0;

	fps = es_get_fps(&seq);
	es_set_timecode(&gop, 0, fps);
	es_set_broken_link(&gop, 0);

	file = fopen(out_path, "wb");
	if(file == NULL){
		es_clear_sequence_header(&seq);
		es_clear_gop_header(&gop);
		es_clear_frame(&frm);
		return;
	}

	es_write_sequence_header(&seq, file);
	es_write_gop_header(&gop, file);

	if(closed_gop){
		current_temporal_reference = es_get_temporal_reference(&frm);
		n = (input_field_count + 1) / 2;
		n += subtract_temporal_reference(current_temporal_reference, previous_temporal_reference);
		current_temporal_reference -= (n-in);
		es_set_temporal_reference(&frm, current_temporal_reference);
		previous_temporal_reference = current_temporal_reference;
	}else{
		previous_temporal_reference = es_get_temporal_reference(&frm);
		es_set_temporal_reference(&frm, 0);
	}
	
	n = es_get_field_count(&frm);
	input_field_count += n;
	output_field_count += n;

	es_write_frame(&frm, file);

	while(bs_next_packet_prefix(bs)){
		
		current_percent = (int)(input_field_count * 50 / out);
		if(((MPEG2EDIT *)mpeg2edit)->callback(out_path, current_percent) != VF_OK){
			es_clear_sequence_header(&seq);
			es_clear_gop_header(&gop);
			es_clear_frame(&frm);
			
			return;
		}
		
		switch(bs_read_bits(bs, 32)){
		case 0x000001b3:
			if( (input_field_count+1)/2 >= out ){
				goto ES_EDIT_4TH_STEP;
			}
			es_clear_sequence_header(&seq);
			es_read_sequence_header(bs, &seq);
			es_write_sequence_header(&seq, file);
			break;
		case 0x000001b8:
			if( (input_field_count+1)/2 >= out ){
				goto ES_EDIT_4TH_STEP;
			}
			es_clear_gop_header(&gop);
			es_read_gop_header(bs, &gop);
			
			closed_gop = es_get_closed_gop(&gop);
			broken_link = es_get_broken_link(&gop);
			
			current_temporal_reference = 0;
			previous_temporal_reference = 0;
			
			es_set_broken_link(&gop, 0);
			es_set_timecode(&gop, (output_field_count+1)/2, fps);
			es_write_gop_header(&gop, file);
			
			break;
		case 0x00000100:
			es_clear_frame(&frm);
			es_read_frame(bs, &frm);
			
			switch(es_get_coding_type(&frm)){
			case 1:
				if( (input_field_count+1)/2 >= out ){
					goto ES_EDIT_4TH_STEP;
				}
				
				if(broken_link == 1){
					broken_link = 2;
					previous_temporal_reference = es_get_temporal_reference(&frm);
				}else{
					broken_link = 0;
				}
				
				current_temporal_reference = es_get_temporal_reference(&frm);
				current_temporal_reference -= previous_temporal_reference;
				es_set_temporal_reference(&frm, current_temporal_reference);
				
				es_write_frame(&frm, file);
				
				n = es_get_field_count(&frm);
				input_field_count += n;
				output_field_count += n;
				
				break;
			case 2:
				if( (input_field_count+1)/2 >= out ){
					goto ES_EDIT_4TH_STEP;
				}

				broken_link = 0;

				current_temporal_reference = es_get_temporal_reference(&frm);
				current_temporal_reference -= previous_temporal_reference;
				es_set_temporal_reference(&frm, current_temporal_reference);

				es_write_frame(&frm, file);

				n = es_get_field_count(&frm);
				input_field_count += n;
				output_field_count += n;

				break;
			case 3:
				n = es_get_field_count(&frm);
				
				if(broken_link == 0){
					input_field_count += n;
				}

				current_temporal_reference = es_get_temporal_reference(&frm);
				if(current_temporal_reference < previous_temporal_reference){
					es_padding_frame(&frm);
				}else{
					current_temporal_reference -= previous_temporal_reference;
					es_set_temporal_reference(&frm, current_temporal_reference);
					output_field_count += n;
				}

				es_write_frame(&frm, file);
			}
			break;
		default:
			bs_erase_bits(bs, 32);
		}
	}

ES_EDIT_4TH_STEP:

	es_write_sequence_end_code(file);

	es_clear_sequence_header(&seq);
	es_clear_gop_header(&gop);
	es_clear_frame(&frm);
	
	fclose(file);
	
	((MPEG2EDIT *)mpeg2edit)->callback(out_path, 100);
	
	return;
	
}

static void es_close(void *mpeg2edit)
{
	MPEG2EDIT *p;
	BITSTREAM *bs;

	p = (MPEG2EDIT *)mpeg2edit;
	bs = (BITSTREAM *)p->stream;

	bs_close(bs);

	p->stream = NULL;
	p->edit = dummy_edit;
	p->close = dummy_close;
}

static int es_read_sequence_header(BITSTREAM *in, ES_SEQUENCE_HEADER *out)
{
	if(bs_read_bits(in, 32) != 0x000001b3){
		return 0;
	}
	
	out->head = read_list(in, 512, es_judge_read_sequence_header_end);
	out->length = count_list_size(out->head);
	
	return out->length;
}

static int es_read_gop_header(BITSTREAM *in, ES_GOP_HEADER *out)
{
	if(bs_read_bits(in, 32) != 0x000001b8){
		return 0;
	}
	
	out->head = read_list(in, 128, es_judge_read_gop_header_end);
	out->length = count_list_size(out->head);
	
	return out->length;
}

static int es_read_picture(BITSTREAM *in, ES_PICTURE *out)
{
	if(bs_read_bits(in, 32) != 0x00000100){
		return 0;
	}
	
	out->head = read_list(in, 8192, es_judge_read_picture_end);
	out->length = count_list_size(out->head);

	return out->length;
}

static int es_read_frame(BITSTREAM *in, ES_FRAME *out)
{
	int r;

	r = es_read_picture(in, &(out->fst));

	if(es_get_picture_field_count(&(out->fst)) == 1){
		r += es_read_picture(in, &(out->snd));
	}

	return r;
}

static int es_judge_read_sequence_header_end(int code)
{
	if(code == 0x00000100){ /* picture_start_code */
		return 1;
	}else if(code == 0x000001b8){ /* gop_start_code */
		return 1;
	}

	return 0;
}

static int es_judge_read_gop_header_end(int code)
{
	if(code == 0x00000100){ /* picture_start_code */
		return 1;
	}

	return 0;
}

static int es_judge_read_picture_end(int code)
{
	if(code == 0x00000100){ /* picture_start_code */
		return 1;
	}else if(code == 0x000001b3){ /* sequence_start_code */
		return 1;
	}else if(code == 0x000001b8){ /* gop_start_code */
		return 1;
	}else if(code == 0x000001b7){ /* sequence_end_code */
		return 1;
	}

	return 0;
}

static void es_write_sequence_header(ES_SEQUENCE_HEADER *in, FILE *out)
{
	fwrite_list(in->head, out);
}

static void es_write_gop_header(ES_GOP_HEADER *in, FILE *out)
{
	fwrite_list(in->head, out);
}

static void es_write_picture(ES_PICTURE *in, FILE *out)
{
	fwrite_list(in->head, out);
}

static void es_write_frame(ES_FRAME *in, FILE *out)
{
	es_write_picture(&(in->fst), out);
	es_write_picture(&(in->snd), out);
}

static void es_write_sequence_end_code(FILE *out)
{
	unsigned char sequence_end_code[4] = {
		0, 0, 1, 0xb7,
	};

	fwrite(sequence_end_code, 1, 4, out);
}

static void es_clear_sequence_header(ES_SEQUENCE_HEADER *seq)
{
	clear_list(seq->head);
	seq->head = NULL;
	seq->length = 0;
}

static void es_clear_gop_header(ES_GOP_HEADER *gop)
{
	clear_list(gop->head);
	gop->head = NULL;
	gop->length = 0;
}

static void es_clear_picture(ES_PICTURE *pic)
{
	clear_list(pic->head);
	pic->head = NULL;
	pic->length = 0;
}

static void es_clear_frame(ES_FRAME *frm)
{
	es_clear_picture(&(frm->fst));
	es_clear_picture(&(frm->snd));
}

static void es_padding_picture(ES_PICTURE *pic)
{
	padding_list(pic->head, 0);
}

static void es_padding_frame(ES_FRAME *frm)
{
	es_padding_picture(&(frm->fst));

	if(frm->snd.length){
		es_padding_picture(&(frm->snd));
	}
}

static int es_get_fps(ES_SEQUENCE_HEADER *seq)
{
	int r;
	int rate;
	int scale;
	int offset;
	unsigned char code;
	unsigned char pattern[4] = { 0, 0, 1, 0xb5 };

	static const int rate_table[16] = {
		 0, 23976, 24, 25, 2997, 30, 50, 5994,
		60,     0,  0,  0,    0,  0,  0,    0,
	};

	static const int scale_table[16] = {
		 1, 1000, 1, 1, 100, 1, 1, 100,
		 1,    1, 1, 1,   1, 1, 1,   1,
	};

	code = get_byte_list(seq->head, 7);
	rate = rate_table[code & 0xf];
	scale = scale_table[code & 0xf];
	
	offset = find_list(seq->head, 12, pattern, 4);
	
	while(offset > 0){
		if( (get_byte_list(seq->head, offset+4) & 0xf0) == 0x10){
			code = get_byte_list(seq->head, offset+9);
			rate *= (((code >> 5) & 0x3) + 1);
			scale *= (((code >> 3) & 0x3) + 1);
			break;
		}else{
			offset = find_list(seq->head, offset+4, pattern, 4);
		}
	}

	r = (rate+(scale-1))/scale;
	
	return r;
}

static void es_set_timecode(ES_GOP_HEADER *gop, __int64 frame, int fps)
{
	int hh;
	int mm;
	int ss;
	int ff;

	unsigned char c0,c1,c2,c3;

	ff = (int)(frame % fps);
	frame /= fps;
	ss = (int)(frame % 60);
	frame /= 60;
	mm = (int)(frame % 60);
	frame /= 60;
	hh = (int)(frame % 24);

	c0 = (unsigned char)( (hh << 2) + (mm >> 4) );
	c1 = (unsigned char)( ((mm & 0x0f) << 4) + 0x08 + (ss >> 3) );
	c2 = (unsigned char)( ((ss & 7) << 5) + (ff >> 1) );
	c3 = (unsigned char)( (get_byte_list(gop->head, 7) & 0x7f) + ((ff & 1) << 7) );

	set_byte_list(gop->head, 4, c0);
	set_byte_list(gop->head, 5, c1);
	set_byte_list(gop->head, 6, c2);
	set_byte_list(gop->head, 7, c3);
}

static int es_get_closed_gop(ES_GOP_HEADER *gop)
{
	int r;

	r = (get_byte_list(gop->head, 7) & 0x40) >> 6;

	return r;
}

static int es_get_broken_link(ES_GOP_HEADER *gop)
{
	int r;

	r = (get_byte_list(gop->head, 7) & 0x20) >> 5;

	return r;
}

static void es_set_broken_link(ES_GOP_HEADER *gop, int broken_link)
{
	unsigned char c;

	c = (unsigned char)(get_byte_list(gop->head, 7) & 0xdf);

	if(broken_link){
		c |= 0x20;
	}

	set_byte_list(gop->head, 7, c);
}

static int  es_get_picture_temporal_reference(ES_PICTURE *pic)
{
	int r;

	r = get_byte_list(pic->head, 4) << 2;
	r += (get_byte_list(pic->head, 5) >> 6);

	return r;
}

static void es_set_picture_temporal_reference(ES_PICTURE *pic, int temporal_reference)
{
	unsigned char c0;
	unsigned char c1;

	temporal_reference &= 0x000003ff;

	c0 = (unsigned char)(temporal_reference >> 2);
	c1 = (unsigned char)( (get_byte_list(pic->head, 5) & 0x3f) + ((temporal_reference & 3) << 6) );

	set_byte_list(pic->head, 4, c0);
	set_byte_list(pic->head, 5, c1);
}

static int es_get_picture_coding_type(ES_PICTURE *pic)
{
	int r;

	r = (get_byte_list(pic->head, 5) >> 3) & 3;
	
	return r;
}

static int es_get_picture_field_count(ES_PICTURE *pic)
{
	int r;
	int offset;
	int code;

	unsigned char pattern[4] = { 0, 0, 1, 0xb5 };

	r = 2;
	
	offset = find_list(pic->head, 8, pattern, 4);
	
	while(offset > 0){
		if( (get_byte_list(pic->head, offset+4) & 0xf0) == 0x80){
			code = get_byte_list(pic->head, offset+6);
			if( (code & 0x03) != 0x03 ){
				return 1;
			}
			code = get_byte_list(pic->head, offset+7);
			if( (code & 0x02) == 0x02 ){
				return 3;
			}
			break;
		}else{
			offset = find_list(pic->head, offset+4, pattern, 4);
		}
	}

	return r;
}

static int es_get_temporal_reference(ES_FRAME *frm)
{
	return es_get_picture_temporal_reference(&(frm->fst));
}

static void es_set_temporal_reference(ES_FRAME *frm, int temporal_reference)
{
	es_set_picture_temporal_reference(&(frm->fst), temporal_reference);

	if(frm->snd.length){
		es_set_picture_temporal_reference(&(frm->snd), temporal_reference);
	}
}

static int es_get_coding_type(ES_FRAME *frm)
{
	return es_get_picture_coding_type(&(frm->fst));
}

static int es_get_field_count(ES_FRAME *frm)
{
	int r;

	r = es_get_picture_field_count(&(frm->fst));

	if(frm->snd.length){
		r += es_get_picture_field_count(&(frm->snd));
	}

	return r;
}

/*-----------------------------------------------------------------*/
static int ps_open(const char *path, MPEG2EDIT *mpeg2edit)
{
	BITSTREAM *bs;

	bs = bs_open(path);
	if(bs == NULL){
		return 0;
	}

	bs_seek(bs, 0, SEEK_SET);

	mpeg2edit->stream = bs;
	mpeg2edit->edit = ps_edit;
	mpeg2edit->close = ps_close;

	return 1;
}

static void ps_edit(void *mpeg2edit, char *out_path, __int64 in, __int64 out)
{
	BITSTREAM *bs;
	FILE *file;
	
	PS_PACK_LIST_ELEMENT *header;
	PS_PACK_LIST_ELEMENT *header_tail;
	
	PS_PACK_LIST_ELEMENT *other;
	PS_PACK_LIST_ELEMENT *other_tail;

	PS_PACK *system;
	
	PS_PACK *video1;
	PS_PACK *video2;

	PS_PACK *previous;
	PS_PACK *current;

	PS_PROC_VIDEO_OPTION opt;

	STREAM_ID_LIST audio_stream;
	
	int status;

	int stream_id;

	ps_init_proc_video_option(&opt, in, out);
	ps_init_stream_id_list(&audio_stream);
	
	header = NULL;
	header_tail = NULL;
	other = NULL;
	other_tail = NULL;

	system = NULL;
	
	video1 = NULL;
	video2 = NULL;

	stream_id = 0;

	status = PS_WASTE_PACK;
	
	bs = ((MPEG2EDIT *)mpeg2edit)->stream;
	
	bs_seek(bs, 0, SEEK_SET);
	bs_next_packet_prefix(bs);

	current = (PS_PACK *)calloc(1, sizeof(PS_PACK));
	previous = NULL;

	while(current){
		if(((MPEG2EDIT *)mpeg2edit)->callback(out_path, ps_get_current_percent(&opt)) != VF_OK){
			break;
		}
		
		ps_read_pack(bs, current);
		
		ps_set_pack_scr_length(previous, ps_get_system_clock_reference(current)-ps_get_system_clock_reference(previous));
		previous = current;
		
		if( (stream_id & 0xf0) != 0xe0 ){
			stream_id = ps_get_video_stream_id(current);
			ps_set_stream_id_proc_video_option(&opt, stream_id);
		}

		if( ps_find_system_header(current) ){
			if(system){
				ps_clear_pack(system);
				free(system);
			}
			system = current;
		}

		if( ((stream_id & 0xf0) == 0xe0) && ps_find_stream(current, stream_id) ){
			video2 = current;
			if(video1){
				status = ps_proc_video(video1, video2, &opt);
				if(status & PS_OUTPUT_PACK){
					if(system == video2){
						system = (PS_PACK *)calloc(1, sizeof(PS_PACK));
						ps_copy_system_header_pack(video2, system);
					}
					goto PS_EDIT_2ND_STEP;
				}else if(status & PS_HEADER_PACK){
					ps_clear_pack_list(header);
					header = ps_add_pack_list(NULL, video1);
					header_tail = header;
				}else if(status & PS_HEADER_CHAIN_PACK){
					header_tail = ps_add_pack_list(header_tail, video1);
				}else{
					ps_clear_pack(video1);
					free(video1);
				}
			}
			video1 = video2;
			if(system == video1){
				system = (PS_PACK *)calloc(1, sizeof(PS_PACK));
				ps_copy_system_header_pack(video1, system);
			}
		}else if(! ps_find_system_header(current)){
			if(ps_check_pack_chain(other_tail, current)){
				other_tail = ps_add_pack_list(other_tail, current);
			}else{
				ps_clear_pack_list(other);
				other = ps_add_pack_list(NULL, current);
				other_tail = other;
			}
		}
		
		if(! bs_next_packet_prefix(bs) ){
			break;
		}
		
		if( bs_read_bits(bs, 32) == 0x000001ba ){
			current = (PS_PACK *)calloc(1, sizeof(PS_PACK));
		}else{
			current = NULL;
		}
	}

	ps_clear_pack_list(header);
	ps_clear_pack_list(other);

	ps_clear_pack(system);
	free(system);
	
	ps_clear_pack(video1);
	free(video1);

	return;

PS_EDIT_2ND_STEP:

	file = fopen(out_path, "wb");
	if(file == NULL){
		ps_clear_pack_list(header);
		ps_clear_pack_list(other);

		ps_clear_pack(system);
		free(system);
	
		ps_clear_pack(video1);
		free(video1);

		return;
	}

	if(status & PS_HEADER_PACK){
		ps_clear_pack_list(header);
		header = NULL;
		header_tail = NULL;
	}

	current = video1;

	if(header){
		ps_set_pack_list_scr(header, ps_get_system_clock_reference(current)-ps_get_pack_list_scr_length(header));
		current = header->data;
	}

	if( ps_find_system_header(current) ){
		if(system){
			ps_clear_pack(system);
			free(system);
			system = NULL;
		}
	}
	
	if(system){
		ps_set_system_clock_reference(system, ps_get_system_clock_reference(current)-ps_get_pack_scr_length(system));
		ps_write_pack(system, file, &audio_stream);
		ps_clear_pack(system);
		free(system);
	}

	ps_initialize_video_packet(current, stream_id);
	
	ps_write_pack_list(header, file, &audio_stream);
	ps_clear_pack_list(header);

	ps_write_pack(video1, file, &audio_stream);
	ps_clear_pack(video1);
	free(video1);
	video1 = video2;

	ps_write_pack_list(other, file, &audio_stream);

	if(! bs_next_packet_prefix(bs) ){
		ps_clear_pack_list(other);
		ps_write_pack(video1, file, &audio_stream);
		ps_clear_pack(video1);
		free(video1);
		fclose(file);
		return;
	}
		
	if( bs_read_bits(bs, 32) == 0x000001ba ){
		current = (PS_PACK *)calloc(1, sizeof(PS_PACK));
	}else{
		current = NULL;
	}

	while(current){
		if(((MPEG2EDIT *)mpeg2edit)->callback(out_path, ps_get_current_percent(&opt)) != VF_OK){
			ps_clear_pack_list(other);
			ps_clear_pack(video1);
			free(video1);
			fclose(file);
			return;
		}

		ps_read_pack(bs, current);

		ps_set_pack_scr_length(previous, ps_get_system_clock_reference(current)-ps_get_system_clock_reference(previous));
		previous = current;

		if(ps_find_stream(current, stream_id)){
			video2 = current;
			status = ps_proc_video(video1, video2, &opt);
			if(status & PS_OUTPUT_PACK){
				ps_write_pack(video1, file, &audio_stream);
				ps_clear_pack(video1);
				free(video1);
				ps_write_pack_list(other, file, &audio_stream);
				ps_clear_pack_list(other);
				other = NULL;
				other_tail = NULL;
			}else{
				goto PS_EDIT_3RD_STEP;
			}
			video1 = video2;
		}else{
			if(ps_check_pack_chain(other_tail, current)){
				other_tail = ps_add_pack_list(other_tail, current);
			}else{
				ps_clear_pack_list(other);
				other = ps_add_pack_list(NULL, current);
				other_tail = other;
			}
		}

		if(! bs_next_packet_prefix(bs) ){
			break;
		}

		if( bs_read_bits(bs, 32) == 0x000001ba ){
			current = (PS_PACK *)calloc(1, sizeof(PS_PACK));
		}else{
			current = NULL;
		}
	}

	ps_proc_video(video1, NULL, &opt);
	ps_write_pack(video1, file, &audio_stream);
	ps_write_pack_list(other, file, &audio_stream);
	video2 = NULL;

PS_EDIT_3RD_STEP:

	ps_clear_pack_list(other);
	if(video1){
		ps_clear_pack(video1);
		free(video1);
	}
	if(video2){
		ps_clear_pack(video2);
		free(video2);
	}
	
	ps_write_program_end_code(file);

	fclose(file);

	if(!ps_check_empty_stream_id_list(&audio_stream)){
		ps_trim_audio_packet(out_path, &audio_stream);
	}
	
	((MPEG2EDIT *)mpeg2edit)->callback(out_path, 100);

	return;
}

static void ps_close(void *mpeg2edit)
{
	MPEG2EDIT *p;
	BITSTREAM *bs;

	p = (MPEG2EDIT *)mpeg2edit;
	bs = (BITSTREAM *)p->stream;

	bs_close(bs);

	p->stream = NULL;
	p->edit = dummy_edit;
	p->close = dummy_close;
}

static void ps_write_program_end_code(FILE *out)
{
	unsigned char pattern[4] = {
		0, 0, 1, 0xb9,
	};

	fwrite(pattern, 1, 4, out);
}

static int ps_read_pack(BITSTREAM *in, PS_PACK *out)
{
	int r;
	
	if(bs_read_bits(in, 32) != 0x000001ba){ /* pack_start_code */
		return 0;
	}

	out->pack_header = read_list(in, 512, ps_judge_read_pack_header_end);
	out->pes_packet = ps_read_pes_packet(in);

	out->scr_length = 0;

	r = count_list_size(out->pack_header);
	r += count_list_size(out->pes_packet);

	return r;
}

static void ps_write_pack(PS_PACK *in, FILE *out, STREAM_ID_LIST *audio_stream)
{
	__int64 offset;
	
	if(in == NULL){
		return;
	}

	fflush(out);
	offset = _telli64(_fileno(out));

	ps_check_audio_stream(in, offset, audio_stream);
	
	fwrite_list(in->pack_header, out);
	fwrite_list(in->pes_packet, out);
}

static void ps_clear_pack(PS_PACK *pack)
{
	if(pack == NULL){
		return;
	}
	
	clear_list(pack->pack_header);
	pack->pack_header = NULL;

	clear_list(pack->pes_packet);
	pack->pes_packet = NULL;
}

static void ps_copy_system_header_pack(PS_PACK *in, PS_PACK *out)
{
	int length;
	LIST_ELEMENT *packet;

	if( (in == NULL) || (out == NULL) ){
		return;
	}

	out->pack_header = copy_list(in->pack_header);
	out->pes_packet = copy_list(in->pes_packet);

	packet = out->pes_packet;
	while(packet){
		length = packet->length-6;
		packet->data[0] = 0;
		packet->data[1] = 0;
		packet->data[2] = 1;
		packet->data[3] = 0xbe;
		packet->data[4] = (unsigned char)((length >> 8) & 0xff);
		packet->data[5] = (unsigned char)(length & 0xff);
		memset(packet->data+6, 0xff, length);
		packet = (LIST_ELEMENT *)packet->next;
	}
}

static int ps_find_system_header(PS_PACK *pack)
{
	unsigned char pattern[4] = {
		0, 0, 1, 0xbb,
	};
	
	if(pack == NULL){
		return 0;
	}
	
	if(find_list(pack->pack_header, 0, pattern, 4) < 0){
		return 0;
	}

	return 1;
}

static int ps_get_video_stream_id(PS_PACK *pack)
{
	LIST_ELEMENT *current;

	if(pack == NULL){
		return 0;
	}
	
	current = pack->pes_packet;

	while(current){
		if( (current->data[3] & 0xf0) == 0xe0 ){
			return current->data[3];
		}
		current = (LIST_ELEMENT *)current->next;
	}

	return 0;
}

static int ps_find_stream(PS_PACK *pack, int stream_id)
{
	LIST_ELEMENT *current;

	if(pack == NULL){
		return 0;
	}
	
	current = pack->pes_packet;

	while(current){
		if(current->data[3] == stream_id){
			return 1;
		}
		current = (LIST_ELEMENT *)current->next;
	}

	return 0;
}

static __int64 ps_get_system_clock_reference(PS_PACK *pack)
{
	__int64 base,ext;
	
	if(pack == NULL){
		return 0;
	}
	if( (get_byte_list(pack->pack_header, 4) & 0xc0) == 0x40 ){ /* MPEG-2 */
		base  = (get_byte_list(pack->pack_header, 4) >> 3) & 7;
		base <<= 2;
		base += get_byte_list(pack->pack_header, 4) & 3;
		base <<= 8;
		base += get_byte_list(pack->pack_header, 5);
		base <<= 5;
		base += (get_byte_list(pack->pack_header, 6) >> 3) & 0x1f;
		base <<= 2;
		base += get_byte_list(pack->pack_header, 6) & 3;
		base <<= 8;
		base += get_byte_list(pack->pack_header, 7);
		base <<= 5;
		base += (get_byte_list(pack->pack_header, 8) >> 3) & 0x1f;

		ext = get_byte_list(pack->pack_header, 8) & 3;
		ext <<= 7;
		ext += get_byte_list(pack->pack_header, 9) >> 1;
	}else{ /* MPEG-1 */
		base = (get_byte_list(pack->pack_header, 4) >> 1) & 7;
		base <<= 8;
		base += get_byte_list(pack->pack_header, 5);
		base <<= 7;
		base += (get_byte_list(pack->pack_header, 6) >> 1) & 0x7f;
		base <<= 8;
		base += get_byte_list(pack->pack_header, 7);
		base <<= 7;
		base += (get_byte_list(pack->pack_header, 8) >> 1) & 0x7f;

		ext = 0;
	}

	return  (base * 300) + ext;
}

static void ps_set_system_clock_reference(PS_PACK *pack, __int64 scr)
{
	__int64 diff;
	__int64 base;
	__int64 ext;

	unsigned char c4;
	unsigned char c5;
	unsigned char c6;
	unsigned char c7;
	unsigned char c8;
	unsigned char c9;

	LIST_ELEMENT *pes_packet;

	if(pack == NULL){
		return;
	}
	
	diff = scr - ps_get_system_clock_reference(pack);

	base = scr / 300;
	ext = scr % 300;

	if( (get_byte_list(pack->pack_header, 4) & 0xc0) == 0x40 ){ /* MPEG-2 */
		c4 = (unsigned char)(0x40 + (((base >> 30) & 7) << 3) + 4 + ((base >> 28) & 3));
		c5 = (unsigned char)((base >> 20) & 0xff);
		c6 = (unsigned char)((((base >> 15) & 0x1f) << 3) + 4 + ((base >> 13) & 3));
		c7 = (unsigned char)((base >> 5) & 0xff);
		c8 = (unsigned char)(((base & 0x1f) << 3) + 4 + ((ext >> 7) & 3));
		c9 = (unsigned char)(((ext & 0x7f) << 1) + 1);
	}else{
		c4 = (unsigned char)(0x20 + (((base >> 30) & 7) << 1) + 1);
		c5 = (unsigned char)((base >> 22) & 0xff);
		c6 = (unsigned char)((((base >> 15) & 0x7f) << 1) + 1);
		c7 = (unsigned char)((base >> 7) & 0xff);
		c8 = (unsigned char)(((base & 0x7f) << 1) + 1);
		c9 = get_byte_list(pack->pack_header, 9);
	}
	set_byte_list(pack->pack_header, 4, c4);
	set_byte_list(pack->pack_header, 5, c5);
	set_byte_list(pack->pack_header, 6, c6);
	set_byte_list(pack->pack_header, 7, c7);
	set_byte_list(pack->pack_header, 8, c8);
	set_byte_list(pack->pack_header, 9, c9);

	pes_packet = pack->pes_packet;

	while(pes_packet){
		ps_set_clock_reference_pes_packet(pes_packet, diff);
		pes_packet = (LIST_ELEMENT *)pes_packet->next;
	}
}

static __int64 ps_get_pack_scr_length(PS_PACK *pack)
{
	if(pack == NULL){
		return 0;
	}
	
	return pack->scr_length;
}

static void ps_set_pack_scr_length(PS_PACK *pack, __int64 scr_length)
{
	if(pack == NULL){
		return;
	}
	
	pack->scr_length = scr_length;
}

static int ps_proc_video(PS_PACK *current, PS_PACK *next, PS_PROC_VIDEO_OPTION *opt)
{
	BLOCK *b1;
	BLOCK *b2;

	int old_offset;
	int new_offset;
	int code;

	int r;

	static unsigned char start_code[] = {
		0, 0, 1,
	};

	static BLOCK pattern = {
		start_code,
		3,
	};

	b1 = ps_join_stream(current, opt->stream_id);
	b2 = ps_join_stream(next, opt->stream_id);

	old_offset = opt->offset;

	if(opt->step == PS_STEP_END){
		opt->output = PS_WASTE_PACK;
	}

	while( (new_offset = find_block(b1, b2, old_offset, &pattern)) >= 0 ){
		
		if((new_offset+3) < b1->length){
			code = b1->data[new_offset+3];
		}else{
			code = b2->data[new_offset+3-b1->length];
		}
		
		switch(code){
		case 0x00: /* picture start code */
			old_offset = ps_proc_video_picture(b1, b2, old_offset, new_offset, opt);
			break;
		case 0xb3: /* sequence start code */
			old_offset = ps_proc_video_sequence(b1, b2, old_offset, new_offset, opt);
			break;
		case 0xb5:
			old_offset = ps_proc_video_extension(b1, b2, old_offset, new_offset, opt);
			break;
		case 0xb7: /* sequence end code */
			old_offset = ps_proc_video_end(b1, b2, old_offset, new_offset, opt);
			break;
		case 0xb8: /* gop start code */
			old_offset = ps_proc_video_gop(b1, b2, old_offset, new_offset, opt);
			break;
		default: /* other start code */
			old_offset = ps_proc_video_other(b1, b2, old_offset, new_offset+4, opt);
		}
	}

	if(old_offset < b1->length){
		old_offset = ps_proc_video_other(b1, b2, old_offset, b1->length, opt);
	}

	opt->offset = old_offset - b1->length;

	r = PS_WASTE_PACK;
	
	if(opt->sequence & (PS_HEADER_PACK|PS_HEADER_CHAIN_PACK)){
		r |= opt->sequence;
		if(opt->picture){
			opt->sequence = PS_WASTE_PACK;
		}else{
			opt->sequence = PS_HEADER_CHAIN_PACK;
		}
	}
	
	r |= opt->output;

	ps_update_stream(current, opt->stream_id, b1);
	ps_update_stream(next, opt->stream_id, b2);
	delete_block(b1);
	delete_block(b2);

	if(opt->step == PS_STEP_END){
		ps_terminate_video_packet(current, opt->stream_id);
	}
	
	return r;
}

static int ps_judge_read_pack_header_end(int code)
{
	if(code > 0x000001bb){
		return 1;
	}

	return 0;
}

static LIST_ELEMENT *ps_read_pes_packet(BITSTREAM *in)
{
	int n,code;
	int packet_length;
	
	LIST_ELEMENT *r;
	LIST_ELEMENT *current;

	const int max_packet_length = 65542;

	code = bs_read_bits(in, 32);
	if( ((code & 0xffffff00) != 0x00000100) || ((code & 0x000000ff) < 0xbc) ){
		return NULL;
	}

	current = new_list_element(NULL, max_packet_length);
	if(current == NULL){
		return NULL;
	}
	r = current;
	
	do{
		while(current->length < 6){
			n = bs_read(in, current->data+current->length, 6-current->length);
			current->length += n;
		}

		packet_length = (current->data[4] << 8) + current->data[5];
		packet_length += 6; /* packet_prefix(3) + stream_id(1) + data_length(2) */

		while(current->length < packet_length){
			n = bs_read(in, current->data+current->length, packet_length-current->length);
			current->length += n;
		}

		code = bs_read_bits(in, 32);

		current->data = (unsigned char *)realloc(current->data, current->length);
		
		if( ((code & 0xffffff00) == 0x00000100) && ((code & 0x000000ff) > 0xbb) ){
			current = new_list_element(current, max_packet_length);
		}else{
			current = NULL;
		}
	}while(current);

	return r;
}

static void ps_init_proc_video_option(PS_PROC_VIDEO_OPTION *opt, __int64 in, __int64 out)
{
	opt->in = in;
	opt->out = out;

	opt->input_field = 0;
	opt->output_field = 0;

	opt->stream_id = 0;
	
	opt->offset = 0;
	
	opt->sequence = PS_WASTE_PACK;
	opt->picture = 0;

	opt->output = PS_WASTE_PACK;

	opt->level = PS_LEVEL_SEQUENCE;
	opt->step = PS_STEP_START;

	opt->temporal_reference = 0;

	opt->padding = 0;
	
	opt->rate = 30;
	opt->scale = 1;
	opt->fps = 30;

	opt->closed_gop = 0;
	opt->broken_link = 0;
}

static void ps_set_stream_id_proc_video_option(PS_PROC_VIDEO_OPTION *opt, int stream_id)
{
	if( (stream_id & 0xf0) == 0xe0 ){
		opt->stream_id = stream_id;
	}
}

static int ps_get_current_percent(PS_PROC_VIDEO_OPTION *opt)
{
	__int64 r;

	r = opt->input_field * 50 / opt->out;

	return (int)r;
}

static PS_PACK_LIST_ELEMENT *ps_add_pack_list(PS_PACK_LIST_ELEMENT *pos, PS_PACK *data)
{
	PS_PACK_LIST_ELEMENT *r;

	r = (PS_PACK_LIST_ELEMENT *)calloc(1, sizeof(PS_PACK_LIST_ELEMENT));

	r->data = data;
	
	r->prev = pos;

	if( pos != NULL ){
		r->next = pos->next;
		pos->next = r;
	}

	return r;
}

static void ps_write_pack_list(PS_PACK_LIST_ELEMENT *head, FILE *out, STREAM_ID_LIST *audio_stream)
{
	while(head){
		ps_write_pack(head->data, out, audio_stream);
		head = (PS_PACK_LIST_ELEMENT *)head->next;
	}
}

static void ps_clear_pack_list(PS_PACK_LIST_ELEMENT *head)
{
	PS_PACK_LIST_ELEMENT *p;

	while(head){
		ps_clear_pack(head->data);
		free(head->data);
		p = (PS_PACK_LIST_ELEMENT *)head->next;
		free(head);
		head = p;
	}
}

static int ps_check_pack_chain(PS_PACK_LIST_ELEMENT *tail, PS_PACK *next)
{
	__int64 scr;

	if(tail == NULL){
		return 0;
	}

	scr = ps_get_system_clock_reference(tail->data) + ps_get_pack_scr_length(tail->data);
	if(scr == ps_get_system_clock_reference(next)){
		return 1;
	}

	return 0;
}

static __int64 ps_get_pack_list_scr_length(PS_PACK_LIST_ELEMENT *head)
{
	__int64 r;

	r = 0;
	while(head){
		r += ps_get_pack_scr_length(head->data);
		head = (PS_PACK_LIST_ELEMENT *)head->next;
	}

	return r;
}
	
static void ps_set_pack_list_scr(PS_PACK_LIST_ELEMENT *head, __int64 scr)
{
	while(head){
		ps_set_system_clock_reference(head->data, scr);
		scr += ps_get_pack_scr_length(head->data);
		head = (PS_PACK_LIST_ELEMENT *)head->next;
	}
}

static void ps_set_clock_reference_pes_packet(LIST_ELEMENT *pes_packet, __int64 diff)
{
	static const unsigned char id_table[] = {
		0, 1, 0, 0,             /* 0xbc - 0xbf */
		1, 1, 1, 1, 1, 1, 1, 1, /* 0xc0 - 0xc7 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 0xc8 - 0xcf */
		1, 1, 1, 1, 1, 1, 1, 1, /* 0xd0 - 0xd7 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 0xd8 - 0xdf */
		1, 1, 1, 1, 1, 1, 1, 1, /* 0xe0 - 0xe7 */
		1, 1, 1, 1, 1, 1, 1, 1, /* 0xe8 - 0xef */
		0, 0, 0, 1, 1, 1, 1, 1, /* 0xf0 - 0xf7 */
		0, 1, 0, 0, 0, 0, 0, 0, /* 0xf8 - 0xff */
	};

	int id;
	unsigned char *p;
	
	int offset;
	int pts_dts_flag;
	int escr_flag;

	__int64 base;
	__int64 ext;
	__int64 cr;

	if(pes_packet->length < 9){
		return;
	}

	p = pes_packet->data;

	id = p[3];

	if(id < 0xbc){
		return;
	}

	if(!id_table[id-0xbc]){
		return;
	}

	if( (p[6] & 0xc0) == 0x80 ){ /* MPEG-2 */
		pts_dts_flag = p[7] >> 6;
		escr_flag = (p[7] >> 5) & 1;

		offset = 9;

		if(pts_dts_flag == 2){ /* pts only */
			base = (p[offset+0] >> 1) & 7;
			base <<= 8;
			base += p[offset+1];
			base <<= 7;
			base += p[offset+2] >> 1;
			base <<= 8;
			base += p[offset+3];
			base <<= 7;
			base += p[offset+4] >> 1;
			base += (diff / 300);
			p[offset+0] = (unsigned char)((p[offset+0] & 0xf0) + ((base >> 30) << 1) + 1);
			p[offset+1] = (unsigned char)((base >> 22) & 0xff);
			p[offset+2] = (unsigned char)((((base >> 15) << 1) & 0xfe) + 1);
			p[offset+3] = (unsigned char)((base >> 7) & 0xff);
			p[offset+4] = (unsigned char)(((base << 1) & 0xfe) + 1);
			offset += 5;
		}else if(pts_dts_flag == 3){ /* pts and dts */
			base = (p[offset+0] >> 1) & 7;
			base <<= 8;
			base += p[offset+1];
			base <<= 7;
			base += p[offset+2] >> 1;
			base <<= 8;
			base += p[offset+3];
			base <<= 7;
			base += p[offset+4] >> 1;
			base += (diff / 300);
			p[offset+0] = (unsigned char)((p[offset+0] & 0xf0) + ((base >> 30) << 1) + 1);
			p[offset+1] = (unsigned char)((base >> 22) & 0xff);
			p[offset+2] = (unsigned char)((((base >> 15) << 1) & 0xfe) + 1);
			p[offset+3] = (unsigned char)((base >> 7) & 0xff);
			p[offset+4] = (unsigned char)(((base << 1) & 0xfe) + 1);
			offset += 5; /* pts */
			
			base = (p[offset+0] >> 1) & 7;
			base <<= 8;
			base += p[offset+1];
			base <<= 7;
			base += p[offset+2] >> 1;
			base <<= 8;
			base += p[offset+3];
			base <<= 7;
			base += p[offset+4] >> 1;
			base += (diff / 300);
			p[offset+0] = (unsigned char)((p[offset+0] & 0xf0) + ((base >> 30) << 1) + 1);
			p[offset+1] = (unsigned char)((base >> 22) & 0xff);
			p[offset+2] = (unsigned char)((((base >> 15) << 1) & 0xfe) + 1);
			p[offset+3] = (unsigned char)((base >> 7) & 0xff);
			p[offset+4] = (unsigned char)(((base << 1) & 0xfe) + 1);
			offset += 5; /* dts */
		}

		if(escr_flag){
			base = (p[offset+0] >> 3) & 7;
			base <<= 2;
			base += p[offset+0] & 3;
			base <<= 8;
			base += p[offset+1];
			base <<= 5;
			base += (p[offset+2] >> 3) & 0x1f;
			base <<= 2;
			base += p[offset+2] & 3;
			base <<= 8;
			base += p[offset+3];
			base <<= 5;
			base += (p[offset+4] >> 3) & 0x1f;

			ext = p[offset+4] & 3;
			ext <<= 7;
			ext += p[offset+5] >> 1;

			cr = base * 300 + ext;
			cr += diff;

			base = cr / 300;
			ext = cr % 300;
			
			p[offset+0] = (unsigned char)((p[offset+0] & 0xc0) + (((base >> 30) & 7) << 3) + 4 + ((base >> 28) & 3));
			p[offset+1] = (unsigned char)((base >> 20) & 0xff);
			p[offset+2] = (unsigned char)((((base >> 15) & 0x1f) << 3) + 4 + ((base >> 13) & 3));
			p[offset+3] = (unsigned char)((base >> 5) & 0xff);
			p[offset+4] = (unsigned char)(((base & 0x1f) << 3) + 4 + ((ext >> 7) & 3));
			p[offset+5] = (unsigned char)(((ext & 0x7f) << 1) + 1);
		}
	}else{ /* MPEG-1 */
		offset = 6;
		while(p[offset] == 0xff){
			offset += 1;
		}
		if( (p[offset] & 0xc0) == 0x40 ){
			offset += 2;
		}
		if( (p[offset] & 0xf0) == 0x20 ){
			base = (p[offset+0] >> 1) & 7;
			base <<= 8;
			base += p[offset+1];
			base <<= 7;
			base += (p[offset+2] >> 1) & 0x7f;
			base <<= 8;
			base += p[offset+3];
			base <<= 7;
			base += (p[offset+4] >> 1) & 0x7f;
			cr = base * 300;
			cr += diff;

			base = cr / 300;

			p[offset+0] = (unsigned char)((p[offset+0] & 0xf0) + (((base >> 30) & 7) << 1) + 1);
			p[offset+1] = (unsigned char)((base >> 22) & 0xff);
			p[offset+2] = (unsigned char)((((base >> 15) & 0x7f) << 1) + 1);
			p[offset+3] = (unsigned char)((base >> 7) & 0xff);
			p[offset+2] = (unsigned char)(((base & 0x7f) << 1) + 1);
		}else if( (p[offset] & 0xf0) == 0x30 ){
			base = (p[offset+0] >> 1) & 7;
			base <<= 8;
			base += p[offset+1];
			base <<= 7;
			base += (p[offset+2] >> 1) & 0x7f;
			base <<= 8;
			base += p[offset+3];
			base <<= 7;
			base += (p[offset+4] >> 1) & 0x7f;
			cr = base * 300;
			cr += diff;

			base = cr / 300;

			p[offset+0] = (unsigned char)((p[offset+0] & 0xf0) + (((base >> 30) & 7) << 1) + 1);
			p[offset+1] = (unsigned char)((base >> 22) & 0xff);
			p[offset+2] = (unsigned char)((((base >> 15) & 0x7f) << 1) + 1);
			p[offset+3] = (unsigned char)((base >> 7) & 0xff);
			p[offset+2] = (unsigned char)(((base & 0x7f) << 1) + 1);

			offset += 5;
			
			base = (p[offset+0] >> 1) & 7;
			base <<= 8;
			base += p[offset+1];
			base <<= 7;
			base += (p[offset+2] >> 1) & 0x7f;
			base <<= 8;
			base += p[offset+3];
			base <<= 7;
			base += (p[offset+4] >> 1) & 0x7f;
			cr = base * 300;
			cr += diff;

			base = cr / 300;

			p[offset+0] = (unsigned char)((p[offset+0] & 0xf0) + (((base >> 30) & 7) << 1) + 1);
			p[offset+1] = (unsigned char)((base >> 22) & 0xff);
			p[offset+2] = (unsigned char)((((base >> 15) & 0x7f) << 1) + 1);
			p[offset+3] = (unsigned char)((base >> 7) & 0xff);
			p[offset+2] = (unsigned char)(((base & 0x7f) << 1) + 1);
		}
	}
}

static BLOCK *ps_join_stream(PS_PACK *pack, int stream_id)
{
	BLOCK *r;
	BLOCK *p;
	LIST_ELEMENT *packet;

	int space;
	
	if(pack == NULL){
		return NULL;
	}

	r = (BLOCK *)malloc(sizeof(BLOCK));
	if(r == NULL){
		return NULL;
	}

	r->data = NULL;
	r->length = 0;

	space = 0;
	packet = pack->pes_packet;

	while(packet){
		if(packet->data[3] == stream_id){
			space += packet->length;
		}
		packet = (LIST_ELEMENT *)packet->next;
	}

	r->data = (unsigned char *)calloc(space, 1);
	if(r->data == NULL){
		free(r);
		return NULL;
	}

	packet = pack->pes_packet;

	while(packet){
		if(packet->data[3] == stream_id){
			p = ps_get_pes_packet_data(packet);
			if(p == NULL){
				delete_block(r);
				return NULL;
			}
			memcpy(r->data+r->length, p->data, p->length);
			r->length += p->length;
			delete_block(p);
		}
		packet = (LIST_ELEMENT *)packet->next;
	}

	return r;
}

static void ps_update_stream(PS_PACK *pack, int stream_id, BLOCK *data)
{
	LIST_ELEMENT *packet;
	
	if( (pack == NULL) || (data == NULL) ){
		return;
	}

	packet = pack->pes_packet;

	while(packet){
		if(packet->data[3] == stream_id){
			ps_set_pes_packet_data(packet, data);
		}
		packet = (LIST_ELEMENT *)packet->next;
	}
}

static BLOCK *ps_get_pes_packet_data(LIST_ELEMENT *pes_packet)
{
	BLOCK *r;
	int offset;
	int length;
	unsigned char *p;

	if(pes_packet == NULL){
		return NULL;
	}

	if(pes_packet->length < 9){
		return NULL;
	}

	p = pes_packet->data;

	offset = ps_get_pes_packet_header_length(pes_packet);
	length = ps_get_pes_packet_data_length(pes_packet);

	r = new_block(p+offset, length);
	
	return r;
}

static void ps_set_pes_packet_data(LIST_ELEMENT *pes_packet, BLOCK *data)
{
	int length;
	int offset;

	unsigned char *p;

	if(pes_packet == NULL){
		return;
	}

	if(pes_packet->length < 9){
		return;
	}
	p = pes_packet->data;
	
	offset = ps_get_pes_packet_header_length(pes_packet);
	length = ps_get_pes_packet_data_length(pes_packet);

	memcpy(p+offset, data->data, length);

	if(data->length - length){
		memmove(data->data, data->data+length, data->length-length);
		data->length -= length;
	}else{
		free(data->data);
		data->data = NULL;
		data->length = 0;
	}
}

static int ps_get_pes_packet_data_length(LIST_ELEMENT *pes_packet)
{
	if(pes_packet == NULL){
		return 0;
	}

	if(pes_packet->length < 9){
		return 0;
	}

	if(pes_packet->data[3] == 0xbe){ /* padding stream */
		return 0;
	}

	return pes_packet->length - ps_get_pes_packet_header_length(pes_packet);
}

static int ps_get_pes_packet_header_length(LIST_ELEMENT *pes_packet)
{
	int r;
	
	if(pes_packet == NULL){
		return 0;
	}

	if(pes_packet->length < 9){
		return 6;
	}

	if(pes_packet->data[3] == 0xbe){ /* padding stream */
		return 6;
	}

	if( (pes_packet->data[6] & 0xc0) == 0x80 ){ /* MPEG-2 */
		r = 9+pes_packet->data[8];
	}else{ /* MPEG-1 */
		r = 6;
		while(pes_packet->data[r] == 0xff){
			r += 1;
		}
		if( (pes_packet->data[r] & 0xc0) == 0x40 ){
			r += 2;
		}
		if( (pes_packet->data[r] & 0xf0) == 0x20 ) {
			r += 5;
		}else if( (pes_packet->data[r] & 0xf0) == 0x30 ){
			r += 10;
		}else{
			r += 1;
		}
	}
		
	return r;
}

static int ps_proc_video_picture(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt)
{
	int code,n;
	
	if(ps_check_fill_zero_video(opt)){
		ps_fill_zero_video(b1, b2, old_offset, new_offset);
	}

	if(opt->level == PS_LEVEL_FIELD){
		return new_offset+2;
	}else{
		opt->level = PS_LEVEL_PICTURE;
		code = (get_byte_block(b1, b2, new_offset+5) >> 3) & 3;

		opt->picture = code;
		opt->padding = 0;
		if(code == 1){ /* I picture */
			if(opt->step == PS_STEP_START){
				opt->step = PS_STEP_SEEK;
				opt->input_field = 2;
				code = get_byte_block(b1, b2, new_offset+4) << 2;
				code += (get_byte_block(b1, b2, new_offset+5) >> 6) & 3;
				opt->temporal_reference = code;
				if(opt->closed_gop == 0){
					opt->broken_link = 1;
				}else{
					opt->closed_gop = 2;
				}
				n = subtract_temporal_reference(opt->temporal_reference, 0);
				if( opt->in <= n ){
					opt->step = PS_STEP_OUTPUT;
					opt->output = PS_OUTPUT_PACK;
					opt->output_field = 2;
					if(opt->closed_gop){
						opt->temporal_reference -= (n-opt->in);
					}		
					code -= opt->temporal_reference;
					set_byte_block(b1, b2, new_offset+4, (unsigned char)((code >> 2) & 0xff));
					code = ((code & 3) << 6) + (get_byte_block(b1, b2, new_offset+5) & 0x3f);
					set_byte_block(b1, b2, new_offset+5, (unsigned char)code);
					return new_offset+6;
				}
				return new_offset+2;
			}else if(opt->step == PS_STEP_SEEK){
				if(opt->broken_link == 1){
					opt->broken_link = 2;
				}else{
					opt->broken_link = 0;
				}
				if(opt->closed_gop == 1){
					opt->closed_gop = 2;
				}else{
					opt->closed_gop = 0;
				}
				code = get_byte_block(b1, b2, new_offset+4) << 2;
				code += (get_byte_block(b1, b2, new_offset+5) >> 6) & 3;
				n = (opt->input_field+subtract_temporal_reference(code, opt->temporal_reference)*2+1)/2;
				opt->input_field +=2;
				if( opt->in <= n ){
					opt->output = PS_OUTPUT_PACK;
					opt->output_field = 2;
					opt->step = PS_STEP_OUTPUT;
					if(opt->closed_gop){
						opt->temporal_reference = code - (n-opt->in);
					}else{
						opt->temporal_reference = code;
					}
					code -= opt->temporal_reference;
					set_byte_block(b1, b2, new_offset+4, (unsigned char)((code >> 2) & 0xff));
					code = ((code & 3) << 6) + (get_byte_block(b1, b2, new_offset+5) & 0x3f);
					set_byte_block(b1, b2, new_offset+5, (unsigned char)code);
					return new_offset+6;
				}
				opt->temporal_reference = code;
				return new_offset+2;
			}else if(opt->step == PS_STEP_OUTPUT){
				if( (opt->input_field+1)/2 >= opt->out ){
					opt->step = PS_STEP_END;
					set_byte_block(b1, b2, new_offset+3, 0xb7);
					return new_offset+4;
				}
				code = get_byte_block(b1, b2, new_offset+4) << 2;
				code += (get_byte_block(b1, b2, new_offset+5) >> 6) & 3;
				if(opt->broken_link == 1){
					opt->broken_link = 2;
					opt->temporal_reference = code;
				}else{
					opt->broken_link = 0;
				}

				code -= opt->temporal_reference;
				set_byte_block(b1, b2, new_offset+4, (unsigned char)((code >> 2) & 0xff));
				code = ((code & 3) << 6) + (get_byte_block(b1, b2, new_offset+5) & 0x3f);
				set_byte_block(b1, b2, new_offset+5, (unsigned char)code);
				opt->input_field += 2;
				opt->output_field += 2;
				return new_offset+6;
			}else{
				return new_offset+2;
			}
		}else if(code == 2){/* P picture */
			if(opt->step == PS_STEP_SEEK){
				opt->broken_link = 0;
				opt->closed_gop = 0;
				code = get_byte_block(b1, b2, new_offset+4) << 2;
				code += (get_byte_block(b1, b2, new_offset+5) >> 6) & 3;
				opt->temporal_reference = code;
				opt->input_field += 2;
				return new_offset+2;
			}else if(opt->step == PS_STEP_OUTPUT){
				if( (opt->input_field+1)/2 >= opt->out ){
					opt->step = PS_STEP_END;
					set_byte_block(b1, b2, new_offset+3, 0xb7);
					return new_offset+4;
				}
				opt->broken_link = 0;
				opt->closed_gop = 0;
				code = get_byte_block(b1, b2, new_offset+4) << 2;
				code += (get_byte_block(b1, b2, new_offset+5) >> 6) & 3;
				code -= opt->temporal_reference;
				set_byte_block(b1, b2, new_offset+4, (unsigned char)((code >> 2) & 0xff));
				code = ((code & 3) << 6) + (get_byte_block(b1, b2, new_offset+5) & 0x3f);
				set_byte_block(b1, b2, new_offset+5, (unsigned char)code);
				opt->input_field += 2;
				opt->output_field += 2;
				return new_offset+6;
			}else{
				return new_offset+2;
			}
		}else if(code == 3){/* B picture */
			if(opt->step == PS_STEP_SEEK){
				if(opt->broken_link == 0){
					opt->input_field += 2;
				}
				return new_offset+2;
			}else if(opt->step == PS_STEP_OUTPUT){
				if(opt->broken_link == 0){
					opt->input_field += 2;
				}
				code = get_byte_block(b1, b2, new_offset+4) << 2;
				code += (get_byte_block(b1, b2, new_offset+5) >> 6) & 3;
				if( code < opt->temporal_reference ){
					opt->padding = 1;
					return new_offset+2;
				}
				code -= opt->temporal_reference;
				set_byte_block(b1, b2, new_offset+4, (unsigned char)((code >> 2) & 0xff));
				code = ((code & 3) << 6) + (get_byte_block(b1, b2, new_offset+5) & 0x3f);
				set_byte_block(b1, b2, new_offset+5, (unsigned char)code);
				opt->output_field += 2;
				return new_offset+6;
			}else{
				return new_offset+2;
			}
		}
		return new_offset+2;
	}
}

static int ps_proc_video_sequence(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt)
{
	int code;
	
	static const int rate_table[16] = {
		 0, 23976, 24, 25, 2997, 30, 50, 5994,
		60,     0,  0,  0,    0,  0,  0,    0,
	};

	static const int scale_table[16] = {
		 1, 1000, 1, 1, 100, 1, 1, 100,
		 1,    1, 1, 1,   1, 1, 1,   1,
	};

	if(ps_check_fill_zero_video(opt)){
		ps_fill_zero_video(b1, b2, old_offset, new_offset);
	}

	if(opt->step == PS_STEP_OUTPUT){
		if( (opt->input_field+1)/2 >= opt->out ){
			set_byte_block(b1, b2, new_offset+3, 0xb7);
			opt->step = PS_STEP_END;
			return new_offset+4;
		}
	}

	opt->sequence = PS_HEADER_PACK;
	opt->picture = 0;
	opt->level = PS_LEVEL_SEQUENCE;

	code = get_byte_block(b1, b2, new_offset+7);

	opt->rate = rate_table[code & 0xf];
	opt->scale = scale_table[code & 0xf];

	opt->fps = (opt->rate+opt->scale-1)/opt->scale;

	return new_offset+8;
}

static int ps_proc_video_end(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt)
{
	if(ps_check_fill_zero_video(opt)){
		ps_fill_zero_video(b1, b2, old_offset, new_offset);
	}

	opt->step = PS_STEP_END;
	opt->level = PS_LEVEL_SEQUENCE;
	
	return new_offset+4;
}

static int ps_proc_video_gop(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt)
{
	int code;

	__int64 frame;
	
	int hh;
	int mm;
	int ss;
	int ff;
	
	if(ps_check_fill_zero_video(opt)){
		ps_fill_zero_video(b1, b2, old_offset, new_offset);
	}

	if(opt->level == PS_LEVEL_SEQUENCE){
		opt->level = PS_LEVEL_GOP;
	}else{
		opt->level = PS_LEVEL_PICTURE;
	}

	if(opt->step == PS_STEP_OUTPUT){
		if( (opt->input_field+1)/2 >= opt->out ){
			set_byte_block(b1, b2, new_offset+3, 0xb7);
			opt->step = PS_STEP_END;
			return new_offset+4;
		}
		opt->closed_gop = (get_byte_block(b1, b2, new_offset+7) & 0x40) >> 6;
		opt->broken_link = (get_byte_block(b1, b2, new_offset+7) & 0x20) >> 5;
		opt->temporal_reference = 0;
	}else{
		opt->closed_gop = (get_byte_block(b1, b2, new_offset+7) & 0x40) >> 6;
		opt->broken_link = (get_byte_block(b1, b2, new_offset+7) & 0x20) >> 5;
		opt->temporal_reference = 0;
	}

	frame = (opt->output_field+1)/2;
	ff = (int)(frame % opt->fps);
	frame /= opt->fps;
	ss = (int)(frame % 60);
	frame /= 60;
	mm = (int)(frame % 60);
	frame /= 60;
	hh = (int)(frame % 24);
	code = (hh << 2) + (mm >> 4);
	set_byte_block(b1, b2, new_offset+4, (unsigned char)code);
	code = ((mm & 0x0f) << 4) + 0x08 + (ss >> 3);
	set_byte_block(b1, b2, new_offset+5, (unsigned char)code);
	code = ((ss & 7) << 5) + (ff >> 1);
	set_byte_block(b1, b2, new_offset+6, (unsigned char)code);
	code = (get_byte_block(b1, b2, new_offset+7) & 0x5f) + ((ff & 1) << 7);
	set_byte_block(b1, b2, new_offset+7, (unsigned char)code);

	return new_offset+2;
}

static int ps_proc_video_other(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt)
{
	if(ps_check_fill_zero_video(opt)){
		ps_fill_zero_video(b1, b2, old_offset, new_offset);
	}

	return new_offset;
}

static int ps_proc_video_extension(BLOCK *b1, BLOCK *b2, int old_offset, int new_offset, PS_PROC_VIDEO_OPTION *opt)
{
	int code;
	
	if(ps_check_fill_zero_video(opt)){
		ps_fill_zero_video(b1, b2, old_offset, new_offset);
	}

	if(opt->level == PS_LEVEL_SEQUENCE){
		code = get_byte_block(b1, b2, new_offset+4) & 0xf0;
		if(code == 0x10){
			code = get_byte_block(b1, b2, new_offset+9);
			opt->rate *= (((code >> 5) & 0x3) + 1);
			opt->scale *= (((code >> 3) & 0x3) + 1);
			opt->fps = (opt->rate+opt->scale-1)/opt->scale;
		}
		return new_offset+2;
	}else if(opt->level == PS_LEVEL_PICTURE){
		code = get_byte_block(b1, b2, new_offset+4) & 0xf0;
		if(code == 0x80){
			code = get_byte_block(b1, b2, new_offset+6);
			if( (code & 0x03) != 0x03 ){
				opt->level = PS_LEVEL_FIELD;
				return new_offset+2;
			}
			code = get_byte_block(b1, b2, new_offset+7);
			if( (code & 0x02) == 0x02 ){
				if(opt->picture == 3){
					if(opt->broken_link == 0){
						opt->input_field += 1;
					}else if(opt->padding == 0){
						opt->output_field += 1;
					}
				}else{
					opt->input_field += 1;
					opt->output_field += 1;
				}
			}
		}
		return new_offset+2;
	}else if(opt->level == PS_LEVEL_FIELD){
		code = get_byte_block(b1, b2, new_offset+4) & 0xf0;
		if(code == 0x80){
			code = get_byte_block(b1, b2, new_offset+6);
			if( (code & 0x03) != 0x03 ){
				opt->level = PS_LEVEL_PICTURE;
				return new_offset+2;
			}
		}
		return new_offset+2;
	}else{
		return new_offset+2;
	}
}

static int ps_check_fill_zero_video(PS_PROC_VIDEO_OPTION *opt)
{
	if(opt->step == PS_STEP_START){
		if(opt->level == PS_LEVEL_SEQUENCE){
			return 0;
		}else if(opt->level == PS_LEVEL_GOP){
			return 0;
		}
		return 1;
	}else if(opt->step == PS_STEP_SEEK){
		if(opt->level == PS_LEVEL_SEQUENCE){
			return 0;
		}else if(opt->level == PS_LEVEL_GOP){
			return 0;
		}
		return 1;
	}else if(opt->step == PS_STEP_OUTPUT){
		if(opt->padding){
			return 1;
		}
		return 0;
	}else if(opt->step == PS_STEP_END){
		return 1;
	}

	return 1;
}

static void ps_fill_zero_video(BLOCK *b1, BLOCK *b2, int from, int to)
{
	if(b1 == NULL){
		return;
	}

	if(from < b1->length){
		if(to < b1->length){
			memset(b1->data+from, 0, to-from);
			return;
		}
		memset(b1->data+from, 0, b1->length-from);
		from = 0;
		to -= b1->length;
	}

	if(b2 == NULL){
		return;
	}

	if(from < b2->length){
		if(to < b2->length){
			memset(b2->data, 0, to);
			return;
		}
		memset(b2->data, 0, b2->length);
	}
}

static void ps_initialize_video_packet(PS_PACK *pack, int stream_id)
{
	int offset;
	int length;
	int i,n;
	LIST_ELEMENT *current;
	BLOCK *b;

	static unsigned char sequence_start_code[] = {
		0, 0, 1, 0xb3,
	};

	static BLOCK pattern = {
		sequence_start_code,
		4,
	};

	if(pack == NULL){
		return;
	}
	
	b = ps_join_stream(pack, stream_id);

	offset = find_block(b, NULL, 0, &pattern);

	delete_block(b);

	current = pack->pes_packet;

	while(current){
		if(current->data[3] == stream_id){
			length = ps_get_pes_packet_data_length(current);
			if(offset < length){
				break;
			}
			current->data[3] = 0xbe;
			memset(current->data+6, 0xff, current->length-6);
			offset -= length;
		}
		current = (LIST_ELEMENT *)current->next;
	}

	if(current == NULL){
		return;
	}

	if(offset < 6){
		return;
	}
	
	n = ps_get_pes_packet_header_length(current);
	
	for(i=0;i<n;i++){ /* header copy */
		current->data[offset+i] = current->data[i];
	}

	length = current->length - offset - 6;
	current->data[offset+4] = (unsigned char)((length >> 8) & 0xff);
	current->data[offset+5] = (unsigned char)(length & 0xff);

	length = offset - 6;
	current->data[3] = 0xbe;
	current->data[4] = (unsigned char)((length >> 8) & 0xff);
	current->data[5] = (unsigned char)(length & 0xff);

	memset(current->data+6, 0xff, length);
}

static void ps_terminate_video_packet(PS_PACK *pack, int stream_id)
{
	int offset;
	int length;

	LIST_ELEMENT *current;
	
	BLOCK *b;

	static unsigned char sequence_end_code[] = {
		0, 0, 1, 0xb7,
	};

	static BLOCK pattern = {
		sequence_end_code,
		4,
	};

	if(pack == NULL){
		return;
	}
	
	b = ps_join_stream(pack, stream_id);

	offset = find_block(b, NULL, 0, &pattern) + pattern.length;

	if(offset < 0){
		return;
	}

	delete_block(b);

	current = pack->pes_packet;

	while(current){
		if(current->data[3] == stream_id){
			length = ps_get_pes_packet_data_length(current);
			if(offset < length){
				break;
			}
			offset -= length;
		}
		current = (LIST_ELEMENT *)current->next;
	}

	if(current == NULL){
		// nothing to do
		return;
	}

	offset += (3+1+2+3+current->data[8]);
	length = current->length - offset;

	current->length = offset;
	offset -= 6;
	current->data[4] = (unsigned char)((offset >> 8) & 0xff);
	current->data[5] = (unsigned char)(offset & 0xff);

	if(length > 6){
		current = new_list_element(current, length);
		current->length = length;
		current->data[2] = 1;
		current->data[3] = (unsigned char)stream_id;
	}else{
		current = (LIST_ELEMENT *)current->next;
	}
	
	while(current){
		if(current->data[3] == stream_id){
			length = current->length - 6;
			current->data[3] = 0xbe;
			current->data[4] = (unsigned char)((offset >> 8) & 0xff);
			current->data[5] = (unsigned char)(offset & 0xff);
			memset(current->data+6, 0xff, length);
		}
		current = (LIST_ELEMENT *)current->next;
	}
}

static void ps_init_stream_id_list(STREAM_ID_LIST *list)
{
	if(list == NULL){
		return;
	}

	memset(list, 0, sizeof(STREAM_ID_LIST));
}

static void ps_add_stream_id_list(int id, __int64 offset, STREAM_ID_LIST *list)
{
	if(list == NULL){
		return;
	}

	if(list->head[id&0xff] == 0){
		list->count += 1;
		list->head[id&0xff] = offset;
	}

	list->tail[id&0xff] = offset;
}

static void ps_remove_stream_id_list(int id, STREAM_ID_LIST *list)
{
	if(list == NULL){
		return;
	}

	if(list->head[id&0xff]){
		list->count -= 1;
	}

	list->head[id&0xff] = 0;
	list->tail[id&0xff] = 0;
}

static int ps_check_stream_id_list(int id, STREAM_ID_LIST *list)
{
	if(list == NULL){
		return 0;
	}

	if(list->head[id&0xff]){
		return 1;
	}else{
		return 0;
	}
}

static int ps_check_empty_stream_id_list(STREAM_ID_LIST *list)
{
	if(list->count > 0){
		return 0;
	}

	return 1;
}

static __int64 ps_get_head_offset_stream_id_list(int id, STREAM_ID_LIST *list)
{
	return list->head[id&0xff];
}

static __int64 ps_get_tail_offset_stream_id_list(int id, STREAM_ID_LIST *list)
{
	return list->tail[id&0xff];
}

static void ps_check_audio_stream(PS_PACK *pack, __int64 offset, STREAM_ID_LIST *audio_stream)
{
	LIST_ELEMENT *current;

	if(pack == NULL){
		return;
	}

	if(audio_stream == NULL){
		return;
	}

	offset += pack->pack_header->length;
	current = pack->pes_packet;

	while(current){
		if( (current->data[3] & 0xe0) == 0xc0 ){
			ps_add_stream_id_list(current->data[3], offset, audio_stream);
		}
		offset += current->length;
		current = (LIST_ELEMENT *)current->next;
	}
}

static void ps_trim_audio_packet(const char *path, STREAM_ID_LIST *audio_stream)
{
	int fd;
	int id;
	__int64 head,tail;
	
	fd = _open(path, _O_BINARY|_O_RDWR|_O_SEQUENTIAL);
	for(id=0xc0;id<0xe0;id++){
		if(ps_check_stream_id_list(id, audio_stream)){
			head = ps_get_head_offset_stream_id_list(id, audio_stream);
			tail = ps_get_tail_offset_stream_id_list(id, audio_stream);
			ps_trim_head_audio_packet(fd, id, head);
			ps_trim_tail_audio_packet(fd, id, tail);
			ps_remove_stream_id_list(id, audio_stream);
		}
	}
	_close(fd);
}

static void ps_trim_head_audio_packet(int fd, int stream_id, __int64 offset)
{
	int i;
	int n,max,length;
	int to;
	LIST_ELEMENT pes;
	unsigned char buffer[65542];
	unsigned char *p,*last;
	
	_lseeki64(fd, offset, SEEK_SET);

	n = _read(fd, buffer, sizeof(buffer));
	
	/*
	   memo
	   packet_start_code 0x000001,     3 byte
	   stream_id         0xc0 ` 0xdf, 1 byte
	   length                          2 byte
	   standard_header                 2 byte
	   header_data_length              1 byte
	   extra_header                    header_data_length byte
	   data                            length - 3 - header_data_length byte
	 */

	if(buffer[3] != stream_id){
		/* bug? */
		return;
	}

	max = (buffer[4] << 8) + buffer[5] + 6;
	pes.data = buffer;
	pes.length = max;
	n = ps_get_pes_packet_header_length(&pes);

	p = buffer+n;
	last = buffer+max;
	while(p<last){
		p = as_find_sync(p, last-p);
		if(p == NULL){
			break;
		}
		n = as_get_frame_size(p, last-p);
		if(n == 0){
			p += 1;
			continue;
		}
		if((p+n+1)<last){
			if( (p[n] == 0xff) && ((p[n+1] & 0xe0) == 0xe0) ){
				break;
			}else{
				p += 1;
			}
		}else{
			break;
		}
	}
		       
	if(p == NULL){
		/* give up */
		return;
	}

	n = ps_get_pes_packet_header_length(&pes);
	to = p - buffer;
	if(n == to){
		/* nothing to do */
		return;
	}

	to -= n;
	
	for(i=0;i<n;i++){
		buffer[to+i] = buffer[i];
	}
	length = max-to-6;
	
	buffer[to+4] = (unsigned char)((length >> 8) & 0xff);
	buffer[to+5] = (unsigned char)(length & 0xff);

	length = to-6;

	if(length < 1){
		for(i=0;i<to;i++){
			buffer[i] = 0x00;
		}
	}else{
		buffer[3] = 0xbe;
		buffer[4] = (unsigned char)((length >> 8) & 0xff);
		buffer[5] = (unsigned char)(length & 0xff);
		memset(buffer+6, 0xff, length);
	}
	
	_lseeki64(fd, offset, SEEK_SET);
	_write(fd, buffer, max);

}

static void ps_trim_tail_audio_packet(int fd, int stream_id, __int64 offset)
{
	int i;
	int n,max,length;
	int to;
	unsigned char buffer[65542];
	unsigned char *p,*last;
	LIST_ELEMENT pes;
	
	_lseeki64(fd, offset, SEEK_SET);

	n = _read(fd, buffer, sizeof(buffer));
	
	if(buffer[3] != stream_id){
		/* bug? */
		return;
	}
	
	max = (buffer[4] << 8) + buffer[5] + 6;
	pes.data = buffer;
	pes.length = max;
	n = ps_get_pes_packet_header_length(&pes);

	p = buffer+n;
	last = buffer+max;
	to = 0;
	while(p<last){
		p = as_find_sync(p, last-p);
		if(p == NULL){
			break;
		}
		to = p - buffer;
		n = as_get_frame_size(p, last-p);
		if(n == 0){
			p += 1;
			continue;
		}
		if((p+n+1)<last){
			if( (p[n] == 0xff) && ((p[n+1] & 0xe0) == 0xe0) ){
				p += n;
			}else{
				p += 1;
			}
		}else{
			p += n;
		}
	}

	if(to == 0){
		/* give up */
		return;
	}

	if(p==last){
		/* nothing to do */
		return;
	}

	length = max-to-6;
	
	if(length < 1){
		for(i=to;i<max;i++){
			buffer[i] = 0;
		}
	}else{
		n = to-6;
		buffer[4] = (unsigned char)((n >> 8) & 0xff);
		buffer[5] = (unsigned char)(n & 0xff);
		buffer[to+0] = 0;
		buffer[to+1] = 0;
		buffer[to+2] = 1;
		buffer[to+3] = 0xbe;
		buffer[to+4] = (unsigned char)((length >> 8) & 0xff);
		buffer[to+5] = (unsigned char)(length & 0xff);
		for(i=to+6;i<max;i++){
			buffer[i] = 0xff;
		}
	}

	_lseeki64(fd, offset, SEEK_SET);
	_write(fd, buffer, max);
}

static unsigned char *as_find_sync(unsigned char *buf, int size)
{
	unsigned char *p,*last;

	p = buf;
	last = p+size-1;

	while(p<last){
		if( (p[0] == 0xff) && ((p[1] & 0xe0) == 0xe0) ){
			return p;
		}
		p += 1;
	}

	return NULL;
}

static int as_get_frame_size(unsigned char *header, int size)
{
	int version;
	int layer;
	int bitrate;
	int freq;
	int padding;
	
	static const int version_table[4] = {
		3, 0, 2, 1,
	};

	static const int layer_table[4] = {
		0, 3, 2, 1,
	};

	static const int bitrate_table[4][4][16] = {
		{   /* version 0 */
			{   /* layer 0 */
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			},{ /* layer 1 */
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			},{ /* layer 2 */
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			},{ /* layer 3 */
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			},
		},{ /* version 1 */
			{ 
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			},{
				0,32000,64000,96000,128000,160000,192000,224000,
				256000,288000,320000,352000,384000,416000,448000,0,
			},{
				0,32000,48000,56000,64000,80000,96000,112000,
				128000,160000,192000,224000,256000,320000,384000,0,
			},{
				0,32000,40000,48000,56000,64000,80000,96000,
				112000,128000,160000,192000,224000,256000,320000,0,
			},
		},{ /* version 2 */
			{ 
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			},{
				0,32000,48000,56000,64000,80000,96000,112000,
				128000,144000,160000,176000,192000,224000,256000,0,
			},{
				0,8000,16000,24000,32000,40000,48000,56000,
				64000,80000,96000,112000,128000,144000,160000,0,
			},{
				0,8000,16000,24000,32000,40000,48000,56000,
				64000,80000,96000,112000,128000,144000,160000,0,
			},
		},{ /* version 2.5 */
			{ 
				0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
			},{
				0,32000,48000,56000,64000,80000,96000,112000,
				128000,144000,160000,176000,192000,224000,256000,0,
			},{
				0,8000,16000,24000,32000,40000,48000,56000,
				64000,80000,96000,112000,128000,144000,160000,0,
			},{
				0,8000,16000,24000,32000,40000,48000,56000,
				64000,80000,96000,112000,128000,144000,160000,0,
			},
		},
	};

	static const int freq_table[4][4] = {
		{
			0,0,0,0,
		},{
			44100,48000,32000,0,
		},{
			22050,24000,16000,0,
		},{
			11024,12000,8000,0,
		},
	};

	if( (header[0] != 0xff) || ((header[1] & 0xe0) != 0xe0) ){
		return 0;
	}

	if( size < 4 ){
		return 0;
	}

	version = version_table[(header[1] >> 3) & 3];
	layer = layer_table[(header[1] >> 1) & 3];
	bitrate = bitrate_table[version][layer][header[2] >> 4];
	freq = freq_table[version][(header[2] >> 2) & 3];
	padding = (header[2] >> 1) & 1;

	if(version == 0){
		return 0;
	}

	if(layer == 0){
		return 0;
	}

	if(layer == 1){
		return ((12*bitrate/freq)+padding)*4;
	}else{
		if(version == 1){
			return 144*bitrate/freq+padding;
		}else{
			return 72*bitrate/freq+padding;
		}
	}
}

