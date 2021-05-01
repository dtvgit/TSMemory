/*******************************************************************
                  bit stream low level interface
 *******************************************************************/
#ifndef BITSTREAM_H
#define BITSTREAM_H

#include <stdio.h>

#include "multi_file.h"

#define BITSTREAM_BUFFER_SIZE   524288
#define BITSTREAM_BUFFER_MARGIN 8

typedef struct {
	
	MULTI_FILE *mf;
	char path[FILENAME_MAX];
	
	unsigned char buffer[BITSTREAM_BUFFER_MARGIN*2+BITSTREAM_BUFFER_SIZE];
	unsigned char *current;
	
	unsigned int bits;
	
	int  buffer_size;
	int  bits_rest;

	__int64 file_length;

} BITSTREAM;

#ifdef __cplusplus
extern "C" {
#endif

extern BITSTREAM *bs_open(const char *filename);
extern void bs_close(BITSTREAM *in);
extern int bs_read(BITSTREAM *in, unsigned char *out, int length);
extern int bs_next_packet_prefix(BITSTREAM *in);
extern int bs_prev_packet_prefix(BITSTREAM *in);
extern int bs_read_bits(BITSTREAM *in, int num_of_bits);
extern int bs_erase_bits(BITSTREAM *in, int num_of_bits);
extern int bs_get_bits(BITSTREAM *in, int num_of_bits);
extern int bs_read_next_packet_prefix(BITSTREAM *in, unsigned char *out, int length);
extern __int64 bs_seek(BITSTREAM *in, __int64 offset, int origin);
extern __int64 bs_tell(BITSTREAM *in);	

#ifdef __cplusplus
}
#endif

#endif
