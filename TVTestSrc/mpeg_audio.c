/*******************************************************************
                   MPEG Audio stream decode module
 *******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instance_manager.h"

#define MPEG_AUDIO_C
#include "mpeg_audio.h"

MPEG_AUDIO *open_mpeg_audio(char *path);
void close_mpeg_audio(MPEG_AUDIO *p);
int read_mpeg_audio(MPEG_AUDIO *p, __int64 offset, void *buffer, int length);

MPEG_AUDIO *open_mpeg_audio(char *path)
{
	AUDIO_INFO info;
	MPEG_AUDIO *r;

	r = (MPEG_AUDIO *)malloc(sizeof(MPEG_AUDIO));
	if(r == NULL){
		return r;
	}

	r->as = audio_stream_open(path);
	if(r->as == NULL){
		free(r);
		return NULL;
	}

	r->as->get_info(r->as->stream, &info);

	r->sample = info.sample;
	r->frequency = info.frequency;
	r->channel = info.channel;

	init_pcm_buffer(&(r->pcm));
	init_layer2_work(&(r->work));
	
	InitializeCriticalSection(&(r->lock));

	register_instance(r, (TEARDOWN_PROC)close_mpeg_audio);

	return r;
}

void close_mpeg_audio(MPEG_AUDIO *p)
{
	if (p == NULL) {
		return;
	}
	
	remove_instance(p);
	
	release_pcm_buffer(&(p->pcm));
	p->as->close(p->as);

	DeleteCriticalSection(&(p->lock));

	free(p);
}

int read_mpeg_audio(MPEG_AUDIO *p, __int64 offset, void *buffer, int length)
{
	__int64 head;

	PCM_BUFFER_ELEMENT *elem;
	
	int r,n;
	short *pos;
	unsigned int sync;

	LAYER2_HEADER layer2;

	r = 0;
	pos = (short *)buffer;

	__asm {emms};
	
	EnterCriticalSection(&(p->lock));

	memset(buffer, 0, sizeof(short)*2*length);

	if(!find_sample_pcm_buffer(&(p->pcm), offset)){
		if(p->as->tell(p->as->stream) == offset){
			sync = p->as->next_sync(p->as->stream);
			parse_layer2_header(sync, &layer2);
			elem = new_pcm_buffer_element(offset, 1152, sizeof(short)*(p->channel));
			if(elem){
				n = p->as->read(p->as->stream, p->unit, layer2.framesize);
				if(n != layer2.framesize){
					goto READ_MPEG_AUDIO_END;
				}
				decode_layer2(&layer2, p->unit+4, &(p->work), (short *)(elem->sample));
				add_element_pcm_buffer(&(p->pcm), elem);
			}else{
				goto READ_MPEG_AUDIO_END;
			}
		}else{
			release_pcm_buffer(&(p->pcm));
			head = p->as->seek(p->as->stream, offset);
			init_layer2_work(&(p->work));
			sync = p->as->next_sync(p->as->stream);
			parse_layer2_header(sync, &layer2);
			elem = new_pcm_buffer_element(head, 1152, sizeof(short)*(p->channel));
			if(elem){
				n = p->as->read(p->as->stream, p->unit, layer2.framesize);
				if(n != layer2.framesize){
					goto READ_MPEG_AUDIO_END;
				}
				decode_layer2(&layer2, p->unit+4, &(p->work), (short *)(elem->sample));
				if(offset >= 1152){
					elem->offset += 1152;
					elem->length = 0;
				}
				add_element_pcm_buffer(&(p->pcm), elem);
			}else{
				goto READ_MPEG_AUDIO_END;
			}
		}
	}

	while(length){
		n = read_sample_pcm_buffer(&(p->pcm), offset, pos, length);
		pos += p->channel * n;
		r += n;
		length -= n;
		offset += n;
		if(length){
			sync = p->as->next_sync(p->as->stream);
			parse_layer2_header(sync, &layer2);
			elem = new_pcm_buffer_element(p->pcm.tail->offset+p->pcm.tail->length, 1152, sizeof(short)*(p->channel));
			if(elem){
				n = p->as->read(p->as->stream, p->unit, layer2.framesize);
				if(n != layer2.framesize){
					goto READ_MPEG_AUDIO_END;
				}
				decode_layer2(&layer2, p->unit+4, &(p->work), (short *)(elem->sample));
				add_element_pcm_buffer(&(p->pcm), elem);
			}else{
				goto READ_MPEG_AUDIO_END;
			}
		}
	}

READ_MPEG_AUDIO_END:
	LeaveCriticalSection(&(p->lock));
	return r;
}

