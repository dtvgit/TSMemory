/*******************************************************************
                     endian related interfaces
 *******************************************************************/
#ifndef ENDIAN_H
#define ENDIAN_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int read_le_int32(FILE *in, int *out);
extern int read_le_int64(FILE *in, __int64 *out);
extern int write_le_int32(int in, FILE *out);
extern int write_le_int64(__int64 in, FILE *out);

#ifdef __cplusplus
}
#endif

#endif
