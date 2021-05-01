#ifndef LAYER2_H
#define LAYER2_H

typedef struct {
	int version;
	int layer;
	int has_crc;
	int bitrate;
	int frequency;
	int padding;
	int channel;
	int bound;
	int emphasis;
	int sblimit;
	int framesize;
} LAYER2_HEADER;

typedef struct {
	double area[2048];
	int    offset;
} LAYER2_WORK;

#ifndef LAYER2_C

#ifdef __cplusplus
extern "C" {
#endif

extern void init_layer2_work(LAYER2_WORK *work);
extern int parse_layer2_header(unsigned int sync, LAYER2_HEADER *out);
extern int decode_layer2(LAYER2_HEADER *head, unsigned char *data, LAYER2_WORK *work, short *pcm);

#ifdef __cplusplus
}
#endif

#endif

#endif
