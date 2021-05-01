#ifndef MEMORY_STREAM_H
#define MEMORY_STREAM_H

typedef struct {
	int           bits_num;
	unsigned int  bits;
	unsigned int  count;
	unsigned char *pos;
	unsigned char *last;
} MEMORY_STREAM;

#ifndef MEMORY_STREAM_C

#ifdef __cplusplus
extern "C" {
#endif

extern void ms_set_buffer(MEMORY_STREAM *ms, void *buffer, int size);
extern unsigned int ms_get_bits(MEMORY_STREAM *ms, int num_of_bits);
extern unsigned int ms_read_bits(MEMORY_STREAM *ms, int num_of_bits);
extern void ms_erase_bits(MEMORY_STREAM *ms, int num_of_bits);
extern void *ms_byte_align(MEMORY_STREAM *ms);

#ifdef __cplusplus
}
#endif

#endif

#endif
