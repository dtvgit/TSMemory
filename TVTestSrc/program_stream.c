/*******************************************************************
                 Program Stream treating module
 *******************************************************************/

#include <stdlib.h>
#include <string.h>
#include "pes.h"
#include "bitstream.h"
#include "registry.h"

#define PROGRAM_STREAM_C
#include "program_stream.h"

#define MAX_PACKET_DATA_LENGTH 65536

typedef struct {
	
	BITSTREAM      *bs;
	PES_STREAM_TYPE type;

	unsigned char   buffer[MAX_PACKET_DATA_LENGTH];
	unsigned char  *current;

	unsigned int    data_rest;
	int             packet_start_code;

	PTS_DTS         current_pts_dts;
} PROGRAM_STREAM;

int ps_open(const char *filename, int stream_type);
int ps_close(int in);
int ps_read(int in, void *data, unsigned int count);
__int64 ps_seek(int in, __int64 offset, int origin);
__int64 ps_tell(int in);

static int read_pes_packet(BITSTREAM *in, PES_PACKET *out);
static int check_ps(BITSTREAM *in);

int ps_open(const char *filename, int stream_type)
{
	PROGRAM_STREAM *ps;
	PES_PACKET packet;
	PES_STREAM_TYPE type;

	init_pes_packet(&packet);

	ps = (PROGRAM_STREAM *)calloc(1, sizeof(PROGRAM_STREAM));
	if(ps == NULL){
		return -1;
	}
	
	ps->bs = bs_open(filename);
	if(ps->bs == NULL){
		free(ps);
		return -1;
	}

	if(!check_ps(ps->bs)){
		bs_close(ps->bs);
		return -1;
	}

	ps->type.type = stream_type;

	while(read_pes_packet(ps->bs, &packet)){
		if(extract_pes_stream_type(&packet, &type)){
			if(type.type == ps->type.type){
				ps->type.id = type.id;
				bs_seek(ps->bs, 0, SEEK_SET);
				ps->packet_start_code = 0x100 + packet.stream_id;
				release_pes_packet(&packet);
				return (int)ps;
			}
		}
	}

	bs_close(ps->bs);
	free(ps);
	release_pes_packet(&packet);
	
	return -1;
}

int  ps_close(int in)
{
	PROGRAM_STREAM *ps;

	ps = (PROGRAM_STREAM *)in;

	if(ps){
		if(ps->bs){
			bs_close(ps->bs);
		}
		free(ps);
	}

	return 1;
}

int  ps_read(int in, void *data, unsigned int count)
{
	int r;
	
	PROGRAM_STREAM *ps;
	PES_PACKET packet;
	PES_STREAM_TYPE type;

	ps = (PROGRAM_STREAM *)in;
	init_pes_packet(&packet);
	
	if(ps->data_rest){
		if(ps->data_rest <= count){
			memcpy(data, ps->current, ps->data_rest);
			r = ps->data_rest;
			ps->data_rest = 0;
			
			release_pes_packet(&packet);
			return r;
		}else{
			memcpy(data, ps->current, count);
			ps->data_rest -= count;
			ps->current += count;
			
			release_pes_packet(&packet);
			return count;
		}
	}

	while(read_pes_packet(ps->bs, &packet) ){
		if( extract_pes_stream_type(&packet, &type) ){
			if( (type.type == ps->type.type) && (type.id == ps->type.id) ){
				extract_pes_packet_data(&packet, ps->buffer, &(ps->data_rest) );
				if(ps->data_rest == 0){
					continue;
				}else if(ps->data_rest <= count){
					memcpy(data, ps->buffer, ps->data_rest);
					r = ps->data_rest;
					ps->data_rest = 0;
					
					release_pes_packet(&packet);
					return r;
				}else{
					memcpy(data, ps->buffer, count);
					ps->data_rest -= count;
					ps->current = ps->buffer + count;
					
					release_pes_packet(&packet);
					return count;
				}
			}
		}
	}
	
					
	release_pes_packet(&packet);
	return 0;
}

__int64  ps_seek(int in, __int64 offset, int origin)
{
	__int64 n,m;
	PROGRAM_STREAM *ps;
	PES_PACKET      packet;
	PES_STREAM_TYPE type;
	
	ps = (PROGRAM_STREAM *)in;
	init_pes_packet(&packet);

	n = bs_seek(ps->bs, offset, origin);

	while(bs_prev_packet_prefix(ps->bs)){
		if( bs_read_bits(ps->bs, 32) == ps->packet_start_code ){
			n = bs_tell(ps->bs);
			if(read_pes_packet(ps->bs, &packet) == 0){
				bs_seek(ps->bs, n-4, SEEK_SET);
				continue;
			}
			extract_pes_stream_type(&packet, &type);
			if(! ( type.type == ps->type.type && type.id == ps->type.id) ){
				break;
			}
			m = bs_tell(ps->bs);
			if( m < n ){
				break;
			}
				
			extract_pes_packet_data(&packet, ps->buffer, &(ps->data_rest));
			ps->current = ps->buffer + (ps->data_rest - (m - n));
			ps->data_rest = (unsigned int) (m - n);
			
			release_pes_packet(&packet);
			return ps_tell(in);
		}else{
			ps->bs->current -= 4;
		}
	}

	while( read_pes_packet(ps->bs, &packet) ){
		if( extract_pes_stream_type(&packet, &type) ){
			if( (type.type == ps->type.type) && (type.id == ps->type.id) ){
				extract_pes_packet_data(&packet, ps->buffer, &(ps->data_rest));
				ps->current = ps->buffer;
				break;
			}
		}
	}

	release_pes_packet(&packet);
	return ps_tell(in);
}

__int64  ps_tell(int in)
{
	__int64 r;

	PROGRAM_STREAM *ps;

	
	ps = (PROGRAM_STREAM *)in;

	r = bs_tell(ps->bs);
	r -= ps->data_rest;

	return r;
}

static int read_pes_packet(BITSTREAM *in, PES_PACKET *out)
{
	int code, n, m;
	unsigned char buf[6];
	
	do{
		if(! bs_next_packet_prefix(in)){
			return 0;
		}

		code = bs_get_bits(in, 32);
		
	}while(code < 0x1bc);

	release_pes_packet(out);
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 1;
	buf[3] = (unsigned char)( code & 0xff );

	n = bs_get_bits(in, 16);
	buf[4] = (unsigned char)( (n >> 8) & 0xff );
	buf[5] = (unsigned char)( n & 0xff );
	
	append_pes_packet_data(out, buf, sizeof(buf));
	while(n){
		m = bs_read(in, out->data+out->size, n);
		n -= m;
		out->size += m;
		if(m == 0){
			return 0;
		}
	}
	return out->size;
}

static int check_ps(BITSTREAM *in)
{
	int limit;

	limit = get_filecheck_limit();
	while(bs_next_packet_prefix(in)){
		if(bs_get_bits(in, 32) > 0x1b8){
			bs_seek(in, 0, SEEK_SET);
			return 1;
		}
		if(limit < bs_tell(in)){
			break;
		}
	}

	return 0;
}

