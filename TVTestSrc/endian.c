/*******************************************************************
                     endian related functions
 *******************************************************************/
#include <string.h>

#include "endian.h"

int read_le_int32(FILE *in, int *out)
{
	int i;
	int c;
	
	*out = 0;
	for(i=0;i<4;i++){
		c = fgetc(in);
		if(c == EOF){
			return 0;
		}
		*out += ((c & 0xff) << (i*8));
	}

	return 1;
}

int read_le_int64(FILE *in, __int64 *out)
{
	int i;
	int c;
	
	*out = 0;
	for(i=0;i<8;i++){
		c = fgetc(in);
		if(c == EOF){
			return 0;
		}
		*out += (((__int64)(c & 0xff)) << (i*8));
	}

	return 1;
}

int write_le_int32(int in, FILE *out)
{
	int i;
	int c;

	for(i=0;i<4;i++){
		c = (in >> (i * 8)) & 0xff;
		c = fputc(c, out);
		if(c == EOF){
			return 0;
		}
	}

	return 1;
}

int write_le_int64(__int64 in, FILE *out)
{
	int i;
	int c;

	for(i=0;i<8;i++){
		c = (in >> (i * 8)) & 0xff;
		c = fputc(c, out);
		if(c == EOF){
			return 0;
		}
	}

	return 1;
}

	

