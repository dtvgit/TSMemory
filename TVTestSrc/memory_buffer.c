#include <stdlib.h>
#include <string.h>

#include "001.h"

#include "memory_buffer.h"

void mb_init(MEMORY_BUFFER *p)
{
	p->pos = p->buffer = NULL;
	p->size = p->capacity = 0;
}

void mb_release(MEMORY_BUFFER *p)
{
	if(p->buffer){
		free(p->buffer);
	}
	mb_init(p);
}
		
int mb_append(MEMORY_BUFFER *p, void *data, int size)
{
	int m,n;
	int newcap;
	unsigned char *newbuf;

	if(p->buffer){
		if(p->size + size < p->capacity){
			memcpy(p->buffer+p->size, data, size);
			p->size += size;
		}else{
			m = p->size-(p->pos-p->buffer);
			n = m + size;
			if(n < p->capacity){
				memcpy(p->buffer, p->pos, m);
				memcpy(p->buffer+m, data, size);
				p->size = n;
				p->pos = p->buffer;
			}else{
				newcap = (n + 4095) & 0x007ff000;
				newbuf = (unsigned char *)malloc(newcap);
				if(newbuf == NULL){
					return 0;
				}
				memcpy(newbuf, p->pos, m);
				memcpy(newbuf+m, data, size);
				free(p->buffer);
				p->buffer = p->pos = newbuf;
				p->capacity = newcap;
			}
		}
	}else{
		newcap = (size+4095) & 0x007ff000;
		p->buffer = (unsigned char *)malloc(newcap);
		if(p->buffer == NULL){
			return 0;
		}
		p->capacity = newcap;
		memcpy(p->buffer, data, size);
		p->pos = p->buffer;
		p->size = size;
	}

	return 1;
}

unsigned char *mb_next_001(MEMORY_BUFFER *p)
{
	unsigned char *newpos;
	
	newpos = find_next_001(p->pos, p->buffer+p->size);
	if(newpos == NULL){
		if(p->pos < p->buffer+p->size-2){
			p->pos = p->buffer+p->size-2;
		}
	}else{
		p->pos = newpos;
	}

	return newpos;
}

unsigned char *mb_next_fff(MEMORY_BUFFER *p)
{
	unsigned char *last;

	last = p->buffer+p->size-1;

	while(p->pos < last){
		if( (p->pos[0] == 0xff) && ((p->pos[1] & 0xf0) == 0xf0) ){
			return p->pos;
		}
		p->pos += 1;
	}

	return NULL;
}

