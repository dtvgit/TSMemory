/*******************************************************************
                    MPEG VIDEO Stream read module
 *******************************************************************/
#include <stdio.h>
#include <io.h>
#include <fcntl.h>

#include "program_stream.h"
#include "transport_stream.h"
#include "001.h"

#include "multi_file.h"

#define VIDEO_STREAM_C
#include "video_stream.h"

int open_video_stream(char *path, VIDEO_STREAM *out);
int close_video_stream(VIDEO_STREAM *p);
int vs_get_bits(VIDEO_STREAM *in, int num_of_bits);
int vs_read_bits(VIDEO_STREAM *in, int num_of_bits);
int vs_erase_bits(VIDEO_STREAM *in, int num_of_bits);
int vs_next_start_code(VIDEO_STREAM *in);
__int64 video_stream_tell(VIDEO_STREAM *p);
__int64 video_stream_seek(VIDEO_STREAM *p, __int64 offset, int origin);

static __inline int fill_bits(VIDEO_STREAM *p);
static __inline int video_stream_getc(VIDEO_STREAM *p);
static __inline unsigned char *find_next_start_code_in_current_buffer(VIDEO_STREAM *in);
static __inline unsigned char *find_prev_start_code_in_current_buffer(VIDEO_STREAM *in);

typedef int (* RAW_CLOSE)(int);
typedef int (* RAW_READ)(int, void *, unsigned int);
typedef __int64 (* RAW_SEEK)(int, __int64, int);
typedef __int64 (* RAW_TELL)(int);

/*******************************************************************/
int open_video_stream(char *path, VIDEO_STREAM *out)
{
	memset(out, 0, sizeof(VIDEO_STREAM));

	strcpy(out->path, path);

	out->fd = ts_open(path, PES_STREAM_TYPE_VIDEO);
	if(out->fd >= 0){
		out->close = ts_close;
		out->read = ts_read;
		out->seek = ts_seek;
		out->tell = ts_tell;
	}else{
		out->fd = ps_open(path, PES_STREAM_TYPE_VIDEO);
		if(out->fd >= 0){
			out->close = ps_close;
			out->read = ps_read;
			out->seek = ps_seek;
			out->tell = ps_tell;
		}else{
			MULTI_FILE *mf;
			mf = open_multi_file(path);
			if(mf == NULL){
				out->fd = -1;
				return 0;
			}
			out->fd = (int)mf;
			out->close = (RAW_CLOSE)mf->close;
			out->read = (RAW_READ)mf->read;
			out->seek = (RAW_SEEK)mf->seek;
			out->tell = (RAW_TELL)mf->tell;
		}
	}
	
	out->file_length = out->seek(out->fd, 0, SEEK_END);
	out->seek(out->fd, 0, SEEK_SET);

	out->buffer_size = out->read(out->fd, out->buffer+VIDEO_STREAM_BUFFER_MARGIN, VIDEO_STREAM_BUFFER_SIZE);
	out->current = out->buffer+VIDEO_STREAM_BUFFER_MARGIN;
	
	out->bits_rest = 0;
	fill_bits(out);
	
	return 1;
}
/*******************************************************************/
int close_video_stream(VIDEO_STREAM *p)
{
	p->close(p->fd);
	return 1;
}
/*******************************************************************/
int vs_get_bits(VIDEO_STREAM *in, int num_of_bits)
{
	int r;

	r = 0;
	
	while(num_of_bits){
		if(num_of_bits <= in->bits_rest){
			r |= (in->bits >> (in->bits_rest - num_of_bits));
			in->bits_rest -= num_of_bits;
			fill_bits(in);
			return r;
		}else{
			num_of_bits -= in->bits_rest;
			r |= (in->bits << num_of_bits);
			in->bits_rest = 0;
			fill_bits(in);
			if(in->bits_rest == 0){
				num_of_bits = 0;
			}
		}	
	}
	
	return r;
}
/*******************************************************************/
int vs_read_bits(VIDEO_STREAM *in, int num_of_bits)
{
	if(num_of_bits <= in->bits_rest){
		return in->bits >> (in->bits_rest - num_of_bits);
	}else{
		return in->bits << (num_of_bits - in->bits_rest);
	}
}
/*******************************************************************/
int vs_erase_bits(VIDEO_STREAM *in, int num_of_bits)
{
	while(num_of_bits){
		if(num_of_bits <= in->bits_rest){
			in->bits_rest -= num_of_bits;
			fill_bits(in);
			return 1;
		}else{
			num_of_bits -= in->bits_rest;
			in->bits_rest = 0;
			fill_bits(in);
			if(in->bits_rest == 0){
				return 0;
			}
		}
	}

	return 1;
}

/*******************************************************************/
int vs_next_start_code(VIDEO_STREAM *in)
{
	int i,n;
	unsigned char *p;
	
	n = in->bits_rest / 8;

	in->current -= n;
	if(in->current < in->buffer + VIDEO_STREAM_BUFFER_MARGIN){
		for(i=0;i<n;i++){
			in->current[i] = (unsigned char)((in->bits >> ((n-i-1)*8)) & 0xff);
		}
	}
	
	while( (p = find_next_start_code_in_current_buffer(in)) == NULL ){
		memcpy(in->buffer, in->buffer+in->buffer_size, VIDEO_STREAM_BUFFER_MARGIN);
		in->current = in->buffer+VIDEO_STREAM_BUFFER_MARGIN-2;
		in->last_position = in->tell(in->fd);
		in->buffer_size = in->read(in->fd, in->buffer+VIDEO_STREAM_BUFFER_MARGIN, VIDEO_STREAM_BUFFER_SIZE);
		if(in->buffer_size == 0){
			return 0;
		}
	}

	in->current = p;
	in->bits_rest = 0;
	fill_bits(in);

	return 1;
}

/*******************************************************************/
__int64 video_stream_tell(VIDEO_STREAM *p)
{
	__int64 r,n;

	n = p->current - p->buffer - (p->bits_rest / 8);
	if(n < VIDEO_STREAM_BUFFER_MARGIN){
		r = p->last_position;
	}else{
		r = p->tell(p->fd);
		r -= p->buffer_size;
	}
	
	r -= VIDEO_STREAM_BUFFER_MARGIN;
	r += n;

	return r;
}

/*******************************************************************/
__int64 video_stream_seek(VIDEO_STREAM *p, __int64 offset, int origin)
{
	p->seek(p->fd, offset, origin);

	p->buffer_size = p->read(p->fd, p->buffer+VIDEO_STREAM_BUFFER_MARGIN, VIDEO_STREAM_BUFFER_SIZE);
	p->current = p->buffer+VIDEO_STREAM_BUFFER_MARGIN;

	p->bits_rest = 0;
	fill_bits(p);
	
	return video_stream_tell(p);
}

/*-----------------------------------------------------------------*/
static int fill_bits(VIDEO_STREAM *p)
{
	int i,n,c;

	n = sizeof(int)*8 - p->bits_rest;
	n >>= 3;

	p->bits &= ( (1 << p->bits_rest) - 1);

	for(i=0;i<n;i++){
		c = video_stream_getc(p);
		if(c != EOF){
			p->bits <<= 8;
			p->bits |= c;
			p->bits_rest += 8;
		}else{
			return 0;
		}
	}

	return 1;
}
/*-----------------------------------------------------------------*/
static int video_stream_getc(VIDEO_STREAM *p)
{
	int r;

	if(p->current < p->buffer + p->buffer_size + VIDEO_STREAM_BUFFER_MARGIN){
		r = p->current[0];
		p->current += 1;
		return r;
	}else{
		p->last_position = p->tell(p->fd);
		p->buffer_size = p->read(p->fd, p->buffer+VIDEO_STREAM_BUFFER_MARGIN, VIDEO_STREAM_BUFFER_SIZE);
		p->current = p->buffer+VIDEO_STREAM_BUFFER_MARGIN;
		if(p->buffer_size == 0){
			return EOF;
		}
		r = p->current[0];
		p->current += 1;
		return r;
	}
}

/*-----------------------------------------------------------------*/
static unsigned char *find_next_start_code_in_current_buffer(VIDEO_STREAM *in)
{
	unsigned char *last;

	last = in->buffer + VIDEO_STREAM_BUFFER_MARGIN + in->buffer_size;

	return find_next_001(in->current, last);
}

