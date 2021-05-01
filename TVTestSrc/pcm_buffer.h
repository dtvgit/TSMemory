/*******************************************************************
                       pcm buffer interface
 *******************************************************************/
#ifndef PCM_BUFFER_H
#define PCM_BUFFER_H

typedef struct {
	__int64   offset;
	void     *sample;
	int       length;
	int       unit;
	
	void     *prev;
	void     *next;
} PCM_BUFFER_ELEMENT;

typedef struct {
	PCM_BUFFER_ELEMENT *head;
	PCM_BUFFER_ELEMENT *tail;
	
	int max_size_limit;
	int min_size_limit;
} PCM_BUFFER;

#ifndef PCM_BUFFER_C

#ifdef __cplusplus
extern "C" {
#endif

extern PCM_BUFFER_ELEMENT *new_pcm_buffer_element(__int64 offset, int length, int unit);
extern void release_pcm_buffer_element(PCM_BUFFER_ELEMENT *p);

extern void init_pcm_buffer(PCM_BUFFER *buf);
extern void release_pcm_buffer(PCM_BUFFER *buf);
extern void add_element_pcm_buffer(PCM_BUFFER *buf, PCM_BUFFER_ELEMENT *elem);
extern int  find_sample_pcm_buffer(PCM_BUFFER *buf, __int64 offset);
extern int  read_sample_pcm_buffer(PCM_BUFFER *buf, __int64 offset, void *buffer, int length); 

#ifdef __cplusplus
}
#endif

#endif /* PCM_BUFFER_C */

#endif /* PCM_BUFFER_H */
