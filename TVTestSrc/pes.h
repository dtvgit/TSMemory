/*******************************************************************
                       PES treating interface
 *******************************************************************/
#ifndef PES_H
#define PES_H

#include "stream_type.h"

typedef struct {
	int  stream_id;
	int  data_length;
	unsigned char *data;

	int  capacity;
	int  size;
} PES_PACKET;

typedef struct {
	int     type;
	int     id;
} PES_STREAM_TYPE;

typedef struct {
	__int64 pts;
	__int64 dts;
} PTS_DTS;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PES_C
extern void init_pes_packet(PES_PACKET *p);
extern int append_pes_packet_data(PES_PACKET *p, unsigned char *data, int size);
extern void release_pes_packet(PES_PACKET *p);
extern int extract_pes_stream_type(PES_PACKET *p, PES_STREAM_TYPE *type);
extern int extract_pes_pts_dts(PES_PACKET *p, PTS_DTS *pts_dts);
extern unsigned int get_pes_packet_data_length(PES_PACKET *p);
extern int extract_pes_packet_data(PES_PACKET *p, unsigned char *data, unsigned int *length);
extern unsigned char *ref_pes_packet_data(PES_PACKET *p);
#endif

#ifdef __cplusplus
}
#endif

#endif
