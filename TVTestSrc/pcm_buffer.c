/*******************************************************************
                         pcm buffer module
 *******************************************************************/
#include <stdlib.h>
#include <string.h>
#define PCM_BUFFER_C
#include "pcm_buffer.h"

PCM_BUFFER_ELEMENT *new_pcm_buffer_element(__int64 offset, int length, int unit);
void release_pcm_buffer_element(PCM_BUFFER_ELEMENT *p);

void init_pcm_buffer(PCM_BUFFER *buf);
void release_pcm_buffer(PCM_BUFFER *buf);
void add_element_pcm_buffer(PCM_BUFFER *buf, PCM_BUFFER_ELEMENT *elem);
int  find_sample_pcm_buffer(PCM_BUFFER *buf, __int64 offset);
int  read_sample_pcm_buffer(PCM_BUFFER *buf, __int64 offset, void *buffer, int length); 

PCM_BUFFER_ELEMENT *new_pcm_buffer_element(__int64 offset, int length, int unit)
{
	PCM_BUFFER_ELEMENT *r;

	r = (PCM_BUFFER_ELEMENT *)malloc(sizeof(PCM_BUFFER_ELEMENT));
	if(r == NULL){
		return NULL;
	}

	r->offset = offset;
	r->sample = malloc(unit*length);
	if(r->sample == NULL){
		return NULL;
	}
	r->length = length;
	r->unit = unit;
	
	r->prev = NULL;
	r->next = NULL;

	return r;
}

void release_pcm_buffer_element(PCM_BUFFER_ELEMENT *p)
{
	if(p){
		if(p->sample){
			free(p->sample);
		}
		free(p);
	}
}

void init_pcm_buffer(PCM_BUFFER *buf)
{
	buf->head = NULL;
	buf->tail = NULL;

	buf->max_size_limit = 1024*1024;
	buf->min_size_limit =  256*1024;
}

void release_pcm_buffer(PCM_BUFFER *buf)
{
	PCM_BUFFER_ELEMENT *curr,*next;

	curr = buf->head;

	while(curr){
		if(curr->next){
			next = (PCM_BUFFER_ELEMENT *)curr->next;
		}else{
			next = NULL;
		}
		release_pcm_buffer_element(curr);
		curr = next;
	}

	buf->head = NULL;
	buf->tail = NULL;
}

void add_element_pcm_buffer(PCM_BUFFER *buf, PCM_BUFFER_ELEMENT *elem)
{
	int total;
	
	elem->prev = buf->tail;
	if(buf->tail){
		buf->tail->next = elem;
	}
	buf->tail = elem;
	if(buf->head == NULL){
		buf->head = elem;
	}

	total = (int)(buf->tail->offset+buf->tail->length-buf->head->offset);
	while(total > buf->max_size_limit){
		total -= buf->head->length;
		if(total > buf->min_size_limit){
			elem = (PCM_BUFFER_ELEMENT *)buf->head->next;
			elem->prev = NULL;
			release_pcm_buffer_element(buf->head);
			buf->head = elem;
		}else{
			break;
		}
	}
}

int find_sample_pcm_buffer(PCM_BUFFER *buf, __int64 offset)
{
	if( buf->head == NULL ){
		return 0;
	}

	if( (buf->head->offset > offset) || (buf->tail->offset+buf->tail->length < offset) ){
		return 0;
	}

	return 1;
}

int read_sample_pcm_buffer(PCM_BUFFER *buf, __int64 offset, void *buffer, int length)
{
	int r;
	int m,o;

	unsigned char *src;
	unsigned char *dst;
	
	PCM_BUFFER_ELEMENT *elem;

	elem = buf->head;
	dst = (unsigned char *)buffer;
	if(offset < elem->offset){
		m = elem->offset-offset;
		memset(dst, 0, m*elem->unit);
		offset += m;
		dst += m * elem->unit;
	}
	
	while(elem){
		if( (elem->offset <= offset) && (offset < elem->offset+elem->length) ){
			break;
		}
		elem = (PCM_BUFFER_ELEMENT *)elem->next;
	}

	r = 0;
	while(elem){
		o = (int)(offset-elem->offset);
		m = elem->length - o;
		src = (unsigned char *)(elem->sample) + (elem->unit * o);
		if(length > m){
			memcpy(dst, src, (elem->unit * m));
			r += m;
		}else{
			memcpy(dst, src, (elem->unit * length));
			r += length;
			break;
		}
		offset += m;
		dst += elem->unit * m;
		length -= m;

		elem = (PCM_BUFFER_ELEMENT *)elem->next;
	}

	return r;
}
