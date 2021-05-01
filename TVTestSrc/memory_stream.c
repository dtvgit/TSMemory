#define MEMORY_STREAM_C
#include "memory_stream.h"

static void fill_bits(MEMORY_STREAM *ms);

void ms_set_buffer(MEMORY_STREAM *ms, void *buffer, int size)
{
	ms->pos = buffer;
	ms->last = ms->pos + size;
	ms->bits = 0;
	ms->bits_num = 0;
	ms->count = 0;

	fill_bits(ms);
}

unsigned int ms_get_bits(MEMORY_STREAM *ms, int num_of_bits)
{
	int n;
	unsigned int r;

	n = ms->bits_num - num_of_bits;
	r = ms->bits >> n;

	ms->bits_num = n;
	ms->bits &= ( (1 << ms->bits_num) - 1 );

	ms->count += num_of_bits;

	fill_bits(ms);

	return r;
}

unsigned int ms_read_bits(MEMORY_STREAM *ms, int num_of_bits)
{
	return (ms->bits >> (ms->bits_num - num_of_bits));
}

void ms_erase_bits(MEMORY_STREAM *ms, int num_of_bits)
{
	int n;
	
	ms->count += num_of_bits;
	
	if(ms->bits_num > num_of_bits){
		ms->bits_num -= num_of_bits;
		ms->bits &= ( (1 << ms->bits_num) - 1 );
	}else{
		num_of_bits -= ms->bits_num;
		n = num_of_bits % 8;
		ms->pos += (num_of_bits / 8);
		if(ms->pos < ms->last){
			ms->bits = ms->pos[0] & ((1 << ms->bits_num) - 1);
			ms->pos += 1;
		}else{
			ms->bits = 0;
		}
		ms->bits_num = n;
	}		

	fill_bits(ms);
}

void *ms_byte_align(MEMORY_STREAM *ms)
{
	int n,m;
	unsigned char *r;
	n = ms->bits_num / 8;
	m = ms->bits_num % 8;
	
	r = ms->pos-n;
	
	ms_erase_bits(ms, m);

	return r;
}

static void fill_bits(MEMORY_STREAM *ms)
{
	int i,n;

	n = 32 - ms->bits_num;
	n >>= 3;

	for(i=0;i<n;i++){
		ms->bits <<= 8;
		if(ms->pos < ms->last){
			ms->bits |= ms->pos[0];
			ms->pos += 1;
		}
		ms->bits_num += 8;
	}
}

