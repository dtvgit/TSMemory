#ifndef MEMORY_BUFFER_H
#define MEMORY_BUFFER_H

typedef struct {
	unsigned char *buffer;
	unsigned char *pos;

	int size;
	int capacity;
} MEMORY_BUFFER;

#ifdef __cplusplus
extern "C" {
#endif

extern void mb_init(MEMORY_BUFFER *p);
extern void mb_release(MEMORY_BUFFER *p);
extern int mb_append(MEMORY_BUFFER *p, void *data, int size);
extern unsigned char *mb_next_001(MEMORY_BUFFER *p);
extern unsigned char *mb_next_fff(MEMORY_BUFFER *p);

#ifdef __cplusplus
}
#endif

#endif /* MEMORY_BUFFER_H */
