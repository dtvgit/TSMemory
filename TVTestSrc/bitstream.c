/*******************************************************************
                  bit stream low level module
 *******************************************************************/

#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "001.h"

#include "bitstream.h"

static int fill_bits(BITSTREAM *in);
static int bitstream_getc(BITSTREAM *in);

static int fill_buffer_next_data(BITSTREAM *in);
static int fill_buffer_previous_data(BITSTREAM *in);

static unsigned char *find_next_packet_prefix_in_current_buffer(BITSTREAM *in);
static unsigned char *find_prev_packet_prefix_in_current_buffer(BITSTREAM *in);

BITSTREAM *bs_open(const char *filename)
{
	BITSTREAM *ret;

	ret = (BITSTREAM *)calloc(1, sizeof(BITSTREAM));
	if(ret == NULL){
		return NULL;
	}

	strcpy(ret->path, filename);

	ret->mf = open_multi_file(filename);
	if(ret->mf == NULL){
		free(ret);
		return NULL;
	}

	ret->buffer_size = ret->mf->read(ret->mf, ret->buffer+BITSTREAM_BUFFER_MARGIN, BITSTREAM_BUFFER_SIZE);
	if(ret->buffer_size == 0){
		return NULL;
	}

	ret->current = ret->buffer + BITSTREAM_BUFFER_MARGIN;

	ret->bits_rest = 0;
	fill_bits(ret);

	return ret;
}

void bs_close(BITSTREAM *in)
{
	if(in){
		if(in->mf){
			in->mf->close(in->mf);
		}
		free(in);
	}
}

int bs_read(BITSTREAM *in, unsigned char *out, int length)
{
	int i,n,r;
	unsigned char *tail;

	tail = in->buffer + in->buffer_size + BITSTREAM_BUFFER_MARGIN;

	n = (in->bits_rest / 8);
	in->current -= n;

	if(in->current < in->buffer + BITSTREAM_BUFFER_MARGIN){
		for(i=0;i<n;i++){
			in->current[i] = (unsigned char)((in->bits >> ((n-i-1)*8)) & 0xff);
		}
	}
	
	
	r = 0;

	while(length){
		if(in->current+length <= tail){
			
			memcpy(out, in->current, length);
			in->current += length;
			r += length;
			length = 0;
			
		}else{

			n = in->buffer_size - (in->current - in->buffer - BITSTREAM_BUFFER_MARGIN);
			memcpy(out, in->current, n);
			out += n;
			r += n;
			length -= n;

			in->buffer_size = in->mf->read(in->mf, in->buffer+BITSTREAM_BUFFER_MARGIN, BITSTREAM_BUFFER_SIZE);
			in->current = in->buffer + BITSTREAM_BUFFER_MARGIN;
			tail = in->buffer + in->buffer_size + BITSTREAM_BUFFER_MARGIN;
			if(in->buffer_size == 0){
				length = 0;
			}
		}
	}

	in->bits_rest = 0;
	fill_bits(in);
	
	return r;
}

__int64 bs_seek(BITSTREAM *in, __int64 offset, int origin)
{
	if(origin == SEEK_CUR){
		offset += bs_tell(in);
		origin = SEEK_SET;
	}

	in->mf->seek(in->mf, offset, origin);
	
	in->buffer_size = in->mf->read(in->mf, in->buffer+BITSTREAM_BUFFER_MARGIN, BITSTREAM_BUFFER_SIZE);
	in->current = in->buffer+BITSTREAM_BUFFER_MARGIN;
	in->bits_rest = 0;
	fill_bits(in);

	return bs_tell(in);
}

__int64 bs_tell(BITSTREAM *in)
{
	__int64 r;

	r = in->mf->tell(in->mf);
	r -= in->buffer_size;
	r += in->current - (in->buffer + BITSTREAM_BUFFER_MARGIN);
	r -= in->bits_rest / 8;

	return r;
}

int bs_next_packet_prefix(BITSTREAM *in)
{
	int i,n;
	unsigned char *p;
	
	n = (in->bits_rest / 8);
	
	if(in->current-n < in->buffer + BITSTREAM_BUFFER_MARGIN){
		for(i=0;i<n;i++){
			in->current -= 1;
			in->current[0] = (unsigned char)((in->bits >> (i*8)) & 0xff);
		}
	}else{
		in->current -= n;
	}
	
	while( (p = find_next_packet_prefix_in_current_buffer(in)) == NULL ){
		in->current = in->buffer + BITSTREAM_BUFFER_MARGIN + in->buffer_size - 2;
		if(! fill_buffer_next_data(in) ){
			return 0;
		}
	}

	in->current = p;
	in->bits_rest = 0;
	fill_bits(in);

	return 1;
}

int bs_prev_packet_prefix(BITSTREAM *in)
{
	unsigned char *p;
	
	while( (p = find_prev_packet_prefix_in_current_buffer(in)) == NULL ){
		if( 2 < in->current-in->buffer-BITSTREAM_BUFFER_MARGIN ){
			in->current = in->buffer + BITSTREAM_BUFFER_MARGIN + 2;
		}
		if( ! fill_buffer_previous_data(in) ){
			return 0;
		}
	}

	in->current = p;
	in->bits_rest = 0;
	fill_bits(in);

	return 1;
}

int bs_read_bits(BITSTREAM *in, int num_of_bits)
{
	if(num_of_bits <= in->bits_rest){
		return in->bits >> (in->bits_rest - num_of_bits);
	}else{
		return in->bits << (num_of_bits - in->bits_rest);
	}
}

int bs_erase_bits(BITSTREAM *in, int num_of_bits)
{
	while(num_of_bits){
		if(num_of_bits < in->bits_rest){
			in->bits_rest -= num_of_bits;
			in->bits &= ( (1 << in->bits_rest) - 1);
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

int bs_get_bits(BITSTREAM *in, int num_of_bits)
{
	int r;

	r = 0;
	
	while(num_of_bits){
		if(num_of_bits < in->bits_rest){
			r |= (in->bits >> (in->bits_rest - num_of_bits));
			in->bits_rest -= num_of_bits;
			in->bits &= ( (1 << in->bits_rest) - 1);
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

int bs_read_next_packet_prefix(BITSTREAM *in, unsigned char *out, int length)
{
	int i,n;
	int r;
	unsigned char *p;
	
	n = in->bits_rest >> 3;
	
	if(in->current-n < in->buffer + BITSTREAM_BUFFER_MARGIN){
		for(i=0;i<n;i++){
			in->current -= 1;
			in->current[0] = (unsigned char)((in->bits >> (i*8)) & 0xff);
		}
	}else{
		in->current -= n;
	}

	if(n+in->buffer_size == 0){
		return 0;
	}

	r = 0;

	while( (p = find_next_packet_prefix_in_current_buffer(in)) == NULL ){
		n = in->buffer + BITSTREAM_BUFFER_MARGIN + in->buffer_size - 2 - in->current;
		if(n < length){
			memcpy(out+r, in->current, n);
			length -= n;
			r += n;
			in->current += n;
		}else{
			memcpy(out+r, in->current, length);
			r += length;
			in->current += length;
			
			in->bits_rest = 0;
			fill_bits(in);
			return r;
		}
		fill_buffer_next_data(in);
	}

	n = p - in->current;
	if(n < length){
		memcpy(out+r, in->current, n);
		r += n;
		in->current += n;
	}else{
		memcpy(out+r, in->current, length);
		r += length;
		in->current += length;
	}

	in->bits_rest = 0;
	fill_bits(in);

	return r;
}

static int fill_bits(BITSTREAM *in)
{
	int i,n,c;

	n = sizeof(in->bits)*8 - in->bits_rest;
	n >>= 3;

	for(i=0;i<n;i++){
		c = bitstream_getc(in);
		if(c != EOF){
			in->bits <<= 8;
			in->bits |= c & 0xff;
			in->bits_rest += 8;
		}else{
			return 0;
		}
	}

	return 1;
}

static int bitstream_getc(BITSTREAM *in)
{
	int r;

	if(in->current < in->buffer + in->buffer_size + BITSTREAM_BUFFER_MARGIN){
		r = in->current[0];
		in->current += 1;
		return r;
	}else{
		in->buffer_size = in->mf->read(in->mf, in->buffer+BITSTREAM_BUFFER_MARGIN, BITSTREAM_BUFFER_SIZE);
		in->current = in->buffer + BITSTREAM_BUFFER_MARGIN;
		if(in->buffer_size != 0){
			r = in->current[0];
			in->current += 1;
			return r;
		}else{
			return EOF;
		}
	}
}

static int fill_buffer_next_data(BITSTREAM *in)
{
	memcpy(in->buffer, in->buffer+in->buffer_size, BITSTREAM_BUFFER_MARGIN);
	in->current -= in->buffer_size;
	
	in->buffer_size = in->mf->read(in->mf, in->buffer+BITSTREAM_BUFFER_MARGIN, BITSTREAM_BUFFER_SIZE);
	if(in->buffer_size == 0){
		return 0;
	}

	return 1;
}

static int fill_buffer_previous_data(BITSTREAM *in)
{
	__int64 n;
	
	n = in->mf->tell(in->mf) - in->buffer_size;
	
	if( n - BITSTREAM_BUFFER_SIZE < 0 ){
		if(n == 0){
			return 0;
		}
	}else{
		n = BITSTREAM_BUFFER_SIZE;
	}

	memcpy(in->buffer, in->buffer+BITSTREAM_BUFFER_MARGIN, BITSTREAM_BUFFER_MARGIN);

	in->mf->seek(in->mf, - n - in->buffer_size, SEEK_CUR);
	in->buffer_size = in->mf->read(in->mf, in->buffer+BITSTREAM_BUFFER_MARGIN, (unsigned int)n);
	
	memcpy(in->buffer+BITSTREAM_BUFFER_MARGIN+in->buffer_size, in->buffer, BITSTREAM_BUFFER_MARGIN);

	in->current += in->buffer_size;
	
	return 1;
}

static unsigned char *find_next_packet_prefix_in_current_buffer(BITSTREAM *in)
{
	unsigned char *last;

	last = in->buffer + BITSTREAM_BUFFER_MARGIN + in->buffer_size;

	return find_next_001(in->current, last);
}

static unsigned char *find_prev_packet_prefix_in_current_buffer(BITSTREAM *in)
{
	unsigned char *head;

	head = in->buffer+BITSTREAM_BUFFER_MARGIN;

	return find_prev_001(in->current, head);
}
