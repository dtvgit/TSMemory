/*******************************************************************
                        PES treating module
 *******************************************************************/

#include <string.h>
#include <stdlib.h>

#include "001.h"
#include "memory_stream.h"

#define PES_C
#include "pes.h"

typedef struct {
	int     version;
	int     header_length;
	__int64 pts;
	__int64 dts;
}STANDARD_PES_HEADER;

void init_pes_packet(PES_PACKET *p);
int append_pes_packet_data(PES_PACKET *p, unsigned char *data, int size);
void release_pes_packet(PES_PACKET *p);
unsigned int get_pes_packet_data_length(PES_PACKET *p);
int extract_pes_stream_type(PES_PACKET *p, PES_STREAM_TYPE *type);
int extract_pes_pts_dts(PES_PACKET *p, PTS_DTS *pts_dts);
int extract_pes_packet_data(PES_PACKET *p, unsigned char *data, unsigned int *length);
unsigned char *ref_pes_packet_data(PES_PACKET *p);

static unsigned int private_1_stream_data_length(PES_PACKET *p);
static unsigned int audio_stream_data_length(PES_PACKET *p);
static unsigned int video_stream_data_length(PES_PACKET *p);

static int private_1_stream_type(PES_PACKET *p, PES_STREAM_TYPE *type);
static int audio_stream_type(PES_PACKET *p, PES_STREAM_TYPE *type);
static int video_stream_type(PES_PACKET *p, PES_STREAM_TYPE *type);

static int extract_standard_pes_header(PES_PACKET *p, STANDARD_PES_HEADER *sph);

static int append_pes_first_data(PES_PACKET *p, unsigned char *data, int size);
static int append_pes_next_data(PES_PACKET *p, unsigned char *data, int size);
static int append_pes_unspec_data(PES_PACKET *p, unsigned char *data, int size);
	
void init_pes_packet(PES_PACKET *p)
{
	p->stream_id = 0;
	p->data_length = 0;
	p->data = NULL;
	p->capacity = 0;
	p->size = -6;
}

int append_pes_packet_data(PES_PACKET *p, unsigned char *data, int size)
{
	int r;

	r = 0;
	if(p->size < 0){
		r += append_pes_first_data(p, data, size);
		size -= r;
	}

	if(size > 0){
		if(p->data_length){
			r += append_pes_next_data(p, data+r, size);
		}else{
			r += append_pes_unspec_data(p, data+r, size);
		}
	}

	return r;
}

void release_pes_packet(PES_PACKET *p)
{
	if(p->data){
		free(p->data);
	}
	init_pes_packet(p);
}

unsigned int get_pes_packet_data_length(PES_PACKET *p)
{
	if(p->stream_id == 0xbd){
		return private_1_stream_data_length(p);
	}else if(p->stream_id >= 0xc0 && p->stream_id <= 0xdf){
		return audio_stream_data_length(p);
	}else if(p->stream_id >= 0xe0 && p->stream_id <= 0xef){
		return video_stream_data_length(p);
	}else{
		return 0;
	}
}

int extract_pes_stream_type(PES_PACKET *p, PES_STREAM_TYPE *type)
{
	if(p->stream_id == 0xbd){
		return private_1_stream_type(p, type);
	}else if(p->stream_id >= 0xc0 && p->stream_id <= 0xdf){
		return audio_stream_type(p, type);
	}else if(p->stream_id >= 0xe0 && p->stream_id <= 0xef){
		return video_stream_type(p, type);
	}else{
		type->type = PES_STREAM_TYPE_UNKNOWN;
		type->id = -1;
		return 0;
	}
}

int extract_pes_pts_dts(PES_PACKET *p, PTS_DTS *pts_dts)
{
	STANDARD_PES_HEADER sph;

	if( (p == NULL) || (pts_dts == NULL) ){
		return 0;
	}

	extract_standard_pes_header(p, &sph);

	pts_dts->pts = sph.pts;
	pts_dts->dts = sph.dts;

	return 1;
}

int extract_pes_packet_data(PES_PACKET *p, unsigned char *data, unsigned int *length)
{
	if((data == NULL) || (length == NULL) ){
		return 0;
	}

	*length = get_pes_packet_data_length(p);

	memcpy(data, p->data+(p->size-(*length)), *length);

	return *length;
}

unsigned char *ref_pes_packet_data(PES_PACKET *p)
{
	unsigned int len = get_pes_packet_data_length(p);
	return (p->data + p->size - len);
}

static unsigned int private_1_stream_data_length(PES_PACKET *p)
{
	unsigned int r;
	unsigned char *w;
	STANDARD_PES_HEADER sph;

	extract_standard_pes_header(p, &sph);
	w = p->data + sph.header_length;

	r = 0;
	if( (w[0] >= 0x80) && (w[0] <= 0x8f) ){
		/* AC3 stream */
		r = p->size - sph.header_length - 4;
	}else{
		/* unknown stream */
	}

	return r;
}

static unsigned int audio_stream_data_length(PES_PACKET *p)
{
	STANDARD_PES_HEADER sph;

	extract_standard_pes_header(p, &sph);

	return p->size - sph.header_length;
}

static unsigned int video_stream_data_length(PES_PACKET *p)
{
	STANDARD_PES_HEADER sph;

	extract_standard_pes_header(p, &sph);

	return p->size - sph.header_length;
}

static int private_1_stream_type(PES_PACKET *p, PES_STREAM_TYPE *type)
{
	unsigned char *w;
	STANDARD_PES_HEADER sph;

	extract_standard_pes_header(p, &sph);
	w = p->data + sph.header_length;

	if( (w[0] >= 0x80) && (w[0] <= 0x8f) ){
		type->type = PES_STREAM_TYPE_AC3;
		type->id = w[0] & 0x0f;
		return 1;
	}else{
		type->type = PES_STREAM_TYPE_PRIVATE;
		type->id = -1;
		return 0;
	}
}

static int audio_stream_type(PES_PACKET *p, PES_STREAM_TYPE *type)
{
	type->type = PES_STREAM_TYPE_AUDIO;
	type->id = p->stream_id & 0x1f;

	return 1;
}

static int video_stream_type(PES_PACKET *p, PES_STREAM_TYPE *type)
{
	type->type = PES_STREAM_TYPE_VIDEO;
	type->id = p->stream_id & 0x0f;
	
	return 1;
}

static int extract_standard_pes_header(PES_PACKET *p, STANDARD_PES_HEADER *sph)
{
	MEMORY_STREAM ms;
	
	if( (p == NULL) || (sph == NULL) ){
		return 0;
	}

	ms_set_buffer(&ms, p->data, p->data_length);

	if( ms_read_bits(&ms, 2) == 2 ){ /* MPEG-2 */
		int pts_dts_flag;

		ms_erase_bits(&ms, 8);
		pts_dts_flag = ms_get_bits(&ms, 2);
		ms_erase_bits(&ms, 6);
		
		sph->version = 2;
		sph->header_length = ms_get_bits(&ms, 8) + 3;
		
		if(pts_dts_flag == 2){
			ms_erase_bits(&ms, 4);
			sph->pts = ms_get_bits(&ms, 3);
			ms_erase_bits(&ms, 1);
			sph->pts <<= 15;
			sph->pts += ms_get_bits(&ms, 15);
			ms_erase_bits(&ms, 1);
			sph->pts <<= 15;
			sph->pts += ms_get_bits(&ms, 15);
			sph->dts = -1;
		}else if(pts_dts_flag == 3){
			ms_erase_bits(&ms, 4);
			sph->pts = ms_get_bits(&ms, 3);
			ms_erase_bits(&ms, 1);
			sph->pts <<= 15;
			sph->pts += ms_get_bits(&ms, 15);
			ms_erase_bits(&ms, 1);
			sph->pts <<= 15;
			sph->pts += ms_get_bits(&ms, 15);
			ms_erase_bits(&ms, 5);
			sph->dts = ms_get_bits(&ms, 3);
			ms_erase_bits(&ms, 1);
			sph->dts <<= 15;
			sph->dts += ms_get_bits(&ms, 15);
			ms_erase_bits(&ms, 1);
			sph->dts <<= 15;
			sph->dts += ms_get_bits(&ms, 15);
		}else{
			sph->pts = -1;
			sph->dts = -1;
		}
	}else{ /* MPEG-1 */

		sph->version = 1;

		while(ms_read_bits(&ms, 8) == 0xff){
			ms_erase_bits(&ms, 8);
		}

		if(ms_read_bits(&ms, 2) == 1){
			ms_erase_bits(&ms, 16);
		}

		if(ms_read_bits(&ms, 4) == 2){
			ms_erase_bits(&ms, 4);
			sph->pts = ms_get_bits(&ms, 3);
			ms_erase_bits(&ms, 1);
			sph->pts <<= 15;
			sph->pts += ms_get_bits(&ms, 15);
			ms_erase_bits(&ms, 1);
			sph->pts <<= 15;
			sph->pts += ms_get_bits(&ms, 15);
			ms_erase_bits(&ms, 1);
			sph->dts = -1;
		}else if(ms_read_bits(&ms, 4) == 3){
			ms_erase_bits(&ms, 4);
			sph->pts = ms_get_bits(&ms, 3);
			ms_erase_bits(&ms, 1);
			sph->pts <<= 15;
			sph->pts += ms_get_bits(&ms, 15);
			ms_erase_bits(&ms, 1);
			sph->pts <<= 15;
			sph->pts += ms_get_bits(&ms, 15);
			ms_erase_bits(&ms, 5);
			sph->dts = ms_get_bits(&ms, 3);
			ms_erase_bits(&ms, 1);
			sph->dts <<= 15;
			sph->dts += ms_get_bits(&ms, 15);
			ms_erase_bits(&ms, 1);
			sph->dts <<= 15;
			sph->dts += ms_get_bits(&ms, 15);
			ms_erase_bits(&ms, 1);
		}else{
			sph->pts = -1;
			sph->dts = -1;
			ms_erase_bits(&ms, 8);
		}

		sph->header_length = (ms.count+7) / 8;
	}

	return 1;
}

static int append_pes_first_data(PES_PACKET *p, unsigned char *data, int size)
{
	unsigned char *pos,*tmp;

	pos = data;
	
	while((p->size != 0) && ((pos-data) < size)){
		switch(p->size){
		case -6:
			if( (data+size-pos) > 2 ){
				tmp = find_next_001(pos, data+size);
				if(tmp == NULL){
					pos = data+size-2;
				}else{
					pos = tmp;
				}
			}
			if(pos[0] == 0){
				p->size += 1;
			}
			pos += 1;
			break;
		case -5:
			if(pos[0] == 0){
				p->size += 1;
			}else{
				p->size = -6;
			}
			pos += 1;
			break;
		case -4:
			if(pos[0] == 1){
				p->size += 1;
			}else if(pos[0] != 0){
				p->size = -6;
			}
			pos += 1;
			break;
		case -3:
			if(pos[0] > 0xbb){
				p->size += 1;
				p->stream_id = pos[0];
			}else if(pos[0] == 0){
				p->size = -5;
			}else{
				p->size = -6;
			}
			pos += 1;
			break;
		case -2:
			p->size += 1;
			p->data_length = pos[0] << 8;
			pos += 1;
			break;
		case -1:
			p->size += 1;
			p->data_length += pos[0];
			pos += 1;
			break;
		}
	}

	if( (p->size == 0) && (p->data_length != 0) ){
		p->data = (unsigned char *)malloc(p->data_length);
		p->capacity = p->data_length;
	}
	
	return pos-data;
}

static int append_pes_next_data(PES_PACKET *p, unsigned char *data, int size)
{
	int n;
	
	if(p->data == NULL){
		return 0;
	}

	if(p->data_length){
		n = p->data_length - p->size;
		if( size < n ){
			n = size;
		}
		memcpy(p->data+p->size, data, n);
		p->size += n;
		return n;
	}

	return 0;
}

static int append_pes_unspec_data(PES_PACKET *p, unsigned char *data, int size)
{
	int n;
	unsigned char *np;
	unsigned char *last;

	n = p->capacity - p->size;
	if( n < size ){
		np = (unsigned char *)realloc(p->data, p->capacity+65536);
		if(np){
			p->data = np;
			p->capacity += 65536;
		}
	}

	if(p->data == NULL){
		return 0;
	}

	n = p->capacity - p->size;
	if( size < n ){
		n = size;
	}

	np = p->data+p->size;
	last = np + n;
	memcpy(np, data, n);

	if(p->size > 3){
		np -= 3;
	}

	while( (np = find_next_001(np, last)) != NULL ){
		if(np+3 < last){
			if(np[3] > 0xbb){
				n = np - (p->data+p->size);
				break;
			}else{
				np += 3;
			}
		}else{
			break;
		}
	}
	p->size += n;

	return n;
}
	
