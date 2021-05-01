/*******************************************************************
                   MPEG Audio stream read module
 *******************************************************************/
#include <io.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include "bitstream.h"
#include "001.h"
#include "pes.h"
#include "memory_stream.h"
#include "memory_buffer.h"
#include "layer2.h"

#include "audio_stream.h"

typedef struct {
	__int64 back_pts;
	__int64 face_pts;
} PTS_MAP;

typedef struct {

	__int64 filesize;
	__int64 sample;

	__int64 position;

	BITSTREAM *bs;
	int stream_id;

	int frequency;
	int channel;

	PTS_MAP *pts;
	
	MEMORY_BUFFER buffer;
} AUDIO_PS;

static int check_ps(unsigned char *buffer, int size);

static AUDIO_PS *open_ps(char *path);
static void close_ps(void *audio_stream);
static __int64 tell_ps(int stream);
static __int64 seek_ps(int stream, __int64 sample);
static int read_ps(int stream, void *buffer, int size);
static unsigned int next_sync_ps(int stream);
static void get_info_ps(int stream, AUDIO_INFO *info);

static int setup_format_ps(AUDIO_PS *ps);
static int set_start_pts_ps(AUDIO_PS *ps);
static int set_filesize_and_sample_ps(AUDIO_PS *ps);
static int set_unit_pts_ps(AUDIO_PS *ps, int unit);
static int read_pes_packet_ps(BITSTREAM *in, PES_PACKET *out);
static __int64 pts_to_sample(__int64 pts, int rate);
static __int64 sample_to_pts(__int64 sample, int rate);
static int count_audio_frame(PES_PACKET *packet);
static unsigned char *find_sync_current_ps_buffer(AUDIO_PS *ps);

AUDIO_STREAM *audio_stream_open(char *path)
{
	int fd;
	
	AUDIO_STREAM *r;

	int n;
	unsigned char buffer[512*1204];

	fd = _open(path, _O_BINARY|_O_RDONLY|_O_SEQUENTIAL);
	if(fd < 0){
		return NULL;
	}
	n = _read(fd, buffer, sizeof(buffer));
	_close(fd);
	
	r = (AUDIO_STREAM *)calloc(1, sizeof(AUDIO_STREAM));
	if(r == NULL){
		return NULL;
	}
	
	if(check_ps(buffer, n)){
		r->stream = (int)open_ps(path);
		r->close = close_ps;
		r->tell = tell_ps;
		r->seek = seek_ps;
		r->read = read_ps;
		r->next_sync = next_sync_ps;
		r->get_info = get_info_ps;
	}else{
		r->stream = 0;
	}

	if(r->stream == 0){
		free(r);
		r = NULL;
	}

	return r;
}

static int check_ps(unsigned char *buffer, int size)
{
	int n;
	unsigned char *pos;
	unsigned char *last;
	
	if( (buffer == NULL) || (size < 1) ){
		return 0;
	}

	pos = buffer;
	last = pos + size;

	while( (pos = find_next_001(pos, last)) != NULL ){
		if( (pos[3] & 0xe0) == 0xc0 ){
			n = (pos[4] << 8) + pos[5] + 6;
			if( (pos+n+4 < last) && (pos[n] == 0) && (pos[n+1] == 0) && ((pos[n+2] == 1) || (pos[n+2] == 0)) ){
				/*
				 *  packet_start_code + packet_length の先が
				 *  next_start_code_prefix か padding であれば、PS と判定
				 *  TMPGEnc の VCD テンプレートで作成されたストリームでは
				 *  Audio PES packet の次の 3 byte が start_code_prefix
				 *  になっていないので padding byte も認識するように修正
				 */
				return 1;
			}else{
				pos += 4;
			}
		}else{
			pos += 4;
		}
	}

	return 0;
}

static AUDIO_PS *open_ps(char *path)
{
	int i,n;
	AUDIO_PS *r;

	r = (AUDIO_PS *)calloc(1, sizeof(AUDIO_PS));
	if(r == NULL){
		return NULL;
	}

	r->bs = bs_open(path);
	if(r->bs == NULL){
		free(r);
		return NULL;
	}

	n = r->bs->mf->count(r->bs->mf);
	r->pts = (PTS_MAP *)malloc(sizeof(PTS_MAP)*(n+1));
	if(r->pts == NULL){
		bs_close(r->bs);
		free(r);
		return NULL;
	}
	for(i=0;i<n;i++){
		r->pts[i].back_pts = -1;
		r->pts[i].face_pts = 0;
	}

	if(!setup_format_ps(r)){
		free(r->pts);
		bs_close(r->bs);
		free(r);
		return NULL;
	}
	
	if(!set_start_pts_ps(r)){
		free(r->pts);
		bs_close(r->bs);
		free(r);
		return NULL;
	}

	if(!set_filesize_and_sample_ps(r)){
		free(r->pts);
		bs_close(r->bs);
		free(r);
		return NULL;
	}

	mb_init(&(r->buffer));
	seek_ps((int)r, 0);
	
	return r;
}

static void close_ps(void *audio_stream)
{
	AUDIO_STREAM *p;
	AUDIO_PS     *ps;

	if(audio_stream){
		p = (AUDIO_STREAM *)audio_stream;
		ps = (AUDIO_PS *)p->stream;
		bs_close(ps->bs);
		mb_release(&(ps->buffer));
		free(ps->pts);
		free(ps);
		free(p);
	}
}

static __int64 tell_ps(int stream)
{
	AUDIO_PS *ps;

	ps = (AUDIO_PS *)stream;

	return ps->position;
}

static __int64 seek_ps(int stream, __int64 sample)
{
	__int64 first, last, i, m, n;
	__int64 border;
	__int64 emergency;

	PTS_DTS pts_dts;
	PES_PACKET packet;

	AUDIO_PS *ps;

	int size;
	int find;
	int part;

	ps = (AUDIO_PS *)stream;

	mb_release(&(ps->buffer));

	init_pes_packet(&packet);

	find = 0;
	first = 0;
	last  = ps->filesize;
	m = 0;
	memset(&pts_dts, 0, sizeof(pts_dts));

	size = ps->bs->mf->count(ps->bs->mf);
	part = 0;
	while(part<size-1){
		m = pts_to_sample(ps->pts[part+1].face_pts, ps->frequency);
		if(sample < m){
			last = ps->bs->mf->border(ps->bs->mf, part);
			break;
		}
		first = ps->bs->mf->border(ps->bs->mf, part);
		part += 1;
	}
	
	border = last;

	if(sample >= 1152){
		/* hack to cancel work area reset. */
		sample -= 1152;
	}

	emergency = 0;
	while(!find){
		i = first + (last - first)/2;

		if(last-first < 256){
			break;
		}

		bs_seek(ps->bs, i, SEEK_SET);

		while(read_pes_packet_ps(ps->bs, &packet)){
			if(packet.stream_id == ps->stream_id){
				extract_pes_pts_dts(&packet, &pts_dts);
				if(pts_dts.pts >= 0){
					break;
				}
			}
		}

		if( border < bs_tell(ps->bs) ){
			last = i;
			continue;
		}

		if( (packet.stream_id != ps->stream_id) || (pts_dts.pts < 0) ){
			break;
		}

		m = pts_to_sample(pts_dts.pts - ps->pts[part].back_pts + ps->pts[part].face_pts, ps->frequency);
		n = m + count_audio_frame(&packet) * 1152 + 1152;

		if( (m <= sample) && (sample < n) ){
			find = 1;
			break;
		}else if(sample < n){
			last = i;
		}else{
			first = bs_tell(ps->bs);
			emergency = first;
		}
	}

	if(!find){
		bs_seek(ps->bs, emergency, SEEK_SET);
		while(read_pes_packet_ps(ps->bs, &packet)){
			if(packet.stream_id == ps->stream_id){
				extract_pes_pts_dts(&packet, &pts_dts);
				if(pts_dts.pts >= 0){
					break;
				}
			}
		}
		m = pts_to_sample(pts_dts.pts - ps->pts[part].back_pts + ps->pts[part].face_pts, ps->frequency);
	}

	ps->position = m;
	size = get_pes_packet_data_length(&packet);
	mb_append(&(ps->buffer), packet.data+(packet.size-size), size);

	release_pes_packet(&packet);

	return ps->position;
}

static int read_ps(int stream, void *buffer, int size)
{
	AUDIO_PS *ps;
	PES_PACKET packet;

	int n;
	int r;
	
	ps = (AUDIO_PS *)stream;

	r = 0;
	if( ps->buffer.pos+size <= ps->buffer.buffer+ps->buffer.size ){
		memcpy(buffer, ps->buffer.pos, size);
		ps->buffer.pos += size;
		r = size;
	}else{
		init_pes_packet(&packet);
		while(read_pes_packet_ps(ps->bs, &packet)){
			if(packet.stream_id == ps->stream_id){
				n = get_pes_packet_data_length(&packet);
				mb_append(&(ps->buffer), packet.data+(packet.size-n), n);
				if(ps->buffer.pos+size <= ps->buffer.buffer+ps->buffer.size){
					memcpy(buffer, ps->buffer.pos, size);
					ps->buffer.pos += size;
					r = size;
					break;
				}
			}
		}
		release_pes_packet(&packet);
	}
	
	if(r){
		ps->position += 1152;
	}else{
		memset(buffer, 0, size);
	}

	return r;
}

static unsigned int next_sync_ps(int stream)
{
	AUDIO_PS *ps;
	PES_PACKET packet;

	int n;
	unsigned int r;
	unsigned char *pos;

	ps = (AUDIO_PS *)stream;

	init_pes_packet(&packet);
	
	r = 0;
	while( (pos = find_sync_current_ps_buffer(ps)) == NULL ){
		n = 0;
		while(read_pes_packet_ps(ps->bs, &packet)){
			if(packet.stream_id == ps->stream_id){
				n = get_pes_packet_data_length(&packet);
				mb_append(&(ps->buffer), packet.data+(packet.size-n), n);
				break;
			}
		}
		if(n == 0){
			break;
		}
	}

	release_pes_packet(&packet);

	if( pos && (pos+3 < ps->buffer.buffer+ps->buffer.size) ){
		r = (pos[0] << 24) | (pos[1] << 16) | (pos[2] << 8) | pos[3];
	}

	return r;
}

static void get_info_ps(int stream, AUDIO_INFO *info)
{
	AUDIO_PS *ps;

	ps = (AUDIO_PS *)stream;

	info->sample = ps->sample;
	info->frequency = ps->frequency;
	info->channel = ps->channel;
}

static int setup_format_ps(AUDIO_PS *ps)
{
	PES_PACKET packet;
	PES_STREAM_TYPE type;

	LAYER2_HEADER hd;
	
	unsigned char buffer[256*1024];
	unsigned char *p,*last;

	unsigned int sync;
	int n,pos;

	bs_seek(ps->bs, 0, SEEK_SET);

	init_pes_packet(&packet);
	memset(&hd, 0, sizeof(hd));

	ps->stream_id = 0;
	
	while(read_pes_packet_ps(ps->bs, &packet)){
		if(extract_pes_stream_type(&packet, &type)){
			if(type.type == PES_STREAM_TYPE_AUDIO){
				ps->stream_id = packet.stream_id;
				break;
			}
		}
		if(bs_tell(ps->bs) > 8*1024*1024){
			break;
		}
	}

	if(ps->stream_id == 0){
		return 0;
	}

	extract_pes_packet_data(&packet, buffer, (unsigned int *)&pos);

	while(read_pes_packet_ps(ps->bs, &packet)){
		if(pos > 64*1024){
			break;
		}
		if(bs_tell(ps->bs) > 8*1024*1024){
			break;
		}
		if(packet.stream_id == ps->stream_id){
			extract_pes_packet_data(&packet, buffer+pos, (unsigned int *)&n);
			pos += n;
		}
	}

	p = buffer;
	last = p+pos-3;
	n = 0;
	while(p < last){
		if( (p[0] == 0xff) && ((p[1] & 0xf0) == 0xf0) ){
			sync = ((unsigned int)(p[0]) << 24) | ((unsigned int)(p[1]) << 16) | ((unsigned int)(p[2]) << 8) | p[3];
			if( ! parse_layer2_header(sync, &hd) ){
				p += 1;
				n = 0;
				continue;
			}
			if( p + hd.framesize >= last ){
				break;
			}
			if( (p[hd.framesize] == 0xff) && ((p[hd.framesize+1] & 0xf0) == 0xf0) ){
				n += 1;
				p += hd.framesize;
			}else{
				p += 1;
				n = 0;
			}
		}else{
			p += 1;
			n = 0;
		}
	}

	if(n == 0){
		return 0;
	}
	
	ps->frequency = hd.frequency;
	ps->channel = hd.channel;

	return 1;
}

static int set_start_pts_ps(AUDIO_PS *ps)
{
	__int64 audio_start_pts;
	__int64 video_start_pts;
	__int64 video_pts[3];
	
	PTS_DTS pts_dts;
	PES_PACKET packet;
	PES_STREAM_TYPE type;

	unsigned char *buffer;
	unsigned char *pos;
	unsigned char *last;

	int n;
	int closed_gop;

	int video_id;

	buffer = (unsigned char *)malloc(8*1024*1024);
	if(buffer == NULL){
		return 0;
	}
	
	init_pes_packet(&packet);
	
	pos = buffer;
	last = buffer;
	
	bs_seek(ps->bs, 0, SEEK_SET);

	video_id = 0;

	audio_start_pts = -1;
	video_start_pts = -1;
	video_pts[0] = -1;
	video_pts[1] = -1;
	video_pts[2] = -1;
	closed_gop = 0;

	while(read_pes_packet_ps(ps->bs, &packet)){
		if(extract_pes_stream_type(&packet, &type)){
			if(packet.stream_id == ps->stream_id){
				if(audio_start_pts < 0){
					extract_pes_pts_dts(&packet, &pts_dts);
					audio_start_pts = pts_dts.pts;
				}
			}else if(type.type == PES_STREAM_TYPE_VIDEO){
				if(video_id == 0){
					video_id = type.id;
				}else if(video_id != type.id){
					continue;
				}
				if(video_start_pts < 0){
					extract_pes_pts_dts(&packet, &pts_dts);
					extract_pes_packet_data(&packet, last, (unsigned int *)&n);
					last += n;
					if(pts_dts.pts >= 0){
						if(video_pts[0] < 0){
							video_pts[0] = pts_dts.pts;
						}else{
							video_pts[1] = pts_dts.pts;
						}
					}
					while( (pos = find_next_001(pos, last)) != NULL ){
						if(pos+3>=last){
							break;
						}
						if(pos[3] == 0xb8){
							if(pos+7<last){
								closed_gop = (pos[7] >> 6) & 1;
								pos += 7;
							}else{
								break;
							}
						}else if(pos[3] == 0x00){
							if(pos+5<last){
								int type = (pos[5]>>3) & 7;
								if(type == 1){
									if(!closed_gop){
										video_start_pts = video_pts[0];
										break;
									}else{
										video_pts[2] = video_pts[0];
										if(video_pts[1] >= 0){
											video_pts[0] = video_pts[1];
											video_pts[1] = -1;
										}else{
											video_pts[0] = -1;
										}
									}
								}else if(type == 2){
									if(video_pts[2] >= 0){
										video_start_pts = video_pts[2];
										break;
									}else{
										if(video_pts[1] >= 0){
											video_pts[0] = video_pts[1];
											video_pts[1] = -1;
										}else{
											video_pts[0] = -1;
										}
									}
								}else if(type == 3){
									if(video_pts[2] >= 0){
										video_start_pts = video_pts[0];
										break;
									}else{
										if(video_pts[1] >= 0){
											video_pts[0] = video_pts[1];
											video_pts[1] = -1;
										}else{
											video_pts[0] = -1;
										}
									}
								}
								pos += 5;
							}else{
								break;
							}
						}else{
							pos += 3;
						}
					}
					if(pos == NULL){
						pos = last-2;
					}
				}
			}
		}
		if( (audio_start_pts >= 0) && (video_start_pts >= 0) ){
			break;
		}
		if(bs_tell(ps->bs) > 8*1024*1024){
			break;
		}
	}

	free(buffer);
	release_pes_packet(&packet);

	if( (video_start_pts >= 0) && (audio_start_pts >= 0) ){
		ps->pts[0].back_pts = video_start_pts;
	}else{
		ps->pts[0].back_pts = audio_start_pts;
	}

	if(ps->pts[0].back_pts < 0){
		return 0;
	}

	return 1;
}

static int set_filesize_and_sample_ps(AUDIO_PS *ps)
{
	int i,n;

	n = ps->bs->mf->count(ps->bs->mf);

	for(i=0;i<n;i++){
		if(!set_unit_pts_ps(ps, i)){
			return 0;
		}
	}

	ps->filesize = ps->bs->mf->border(ps->bs->mf, n-1);
	ps->sample = pts_to_sample(ps->pts[n].face_pts, ps->frequency);

	return 1;
}

static int set_unit_pts_ps(AUDIO_PS *ps, int unit)
{
	__int64 offset;
	__int64 sample;
	PTS_DTS pts_dts;
	PES_PACKET packet;

	int r;
	int count;

	bs_seek(ps->bs, ps->bs->mf->border(ps->bs->mf, unit), SEEK_SET);
	
	init_pes_packet(&packet);
	memset(&pts_dts, 0, sizeof(&pts_dts));
	r = 0;
	count = 0;
	while(bs_prev_packet_prefix(ps->bs)){
		if( (bs_read_bits(ps->bs, 32)) == 0x100 + ps->stream_id){
			offset = bs_tell(ps->bs);
			if(read_pes_packet_ps(ps->bs, &packet)){
				extract_pes_pts_dts(&packet, &pts_dts);
				if(pts_dts.pts >= 0){
					sample = count_audio_frame(&packet) * 1152;
					if(sample == 0){
						/* stream has probrem
						 * pes packet contains no audio access unit head byte
						 * and pes packet has pts
						 */
						count+=1;
						if(count > 4){
							ps->pts[unit+1].face_pts = pts_dts.pts;
							break;
						}
						bs_seek(ps->bs, offset-4, SEEK_SET);
						continue;
					}
					ps->pts[unit+1].face_pts = pts_dts.pts - ps->pts[unit].back_pts + ps->pts[unit].face_pts;
					ps->pts[unit+1].face_pts += sample_to_pts(sample, ps->frequency);
					r = 1;
					break;
				}else{
					bs_seek(ps->bs, offset-4, SEEK_SET);
				}
			}else{
				bs_seek(ps->bs, offset-4, SEEK_SET);
			}
		}else{
			ps->bs->current -= 4;
		}
	}

	if(pts_dts.pts < 0){
		release_pes_packet(&packet);
		return 0;
	}

	bs_seek(ps->bs, ps->bs->mf->border(ps->bs->mf, unit), SEEK_SET);
	while(read_pes_packet_ps(ps->bs, &packet)){
		if(packet.stream_id == ps->stream_id){
			extract_pes_pts_dts(&packet, &pts_dts);
			if(pts_dts.pts >= 0){
				ps->pts[unit+1].back_pts = pts_dts.pts;
				break;
			}
		}
	}

	offset = ps->pts[unit+1].face_pts - ps->pts[unit].face_pts;
	offset -= ps->pts[unit+1].back_pts - ps->pts[unit].back_pts;

	if(offset < 0){
		offset = 0 - offset;
	}

	if(offset < 450){ /* under 5 msec */
		ps->pts[unit+1].face_pts = ps->pts[unit+1].back_pts - ps->pts[unit].back_pts + ps->pts[unit].face_pts;
	}
	
	release_pes_packet(&packet);
	
	return r;
}

static int read_pes_packet_ps(BITSTREAM *in, PES_PACKET *out)
{
	int code, n, m;
	unsigned char buf[6];
	
	do{
		if(! bs_next_packet_prefix(in)){
			return 0;
		}

		code = bs_get_bits(in, 32);
		
	}while(code < 0x1bc);

	release_pes_packet(out);
	buf[0] = 0;
	buf[1] = 0;
	buf[2] = 1;
	buf[3] = (unsigned char)( code & 0xff );

	n = bs_get_bits(in, 16);
	buf[4] = (unsigned char)( (n >> 8) & 0xff );
	buf[5] = (unsigned char)( n & 0xff );
	
	append_pes_packet_data(out, buf, sizeof(buf));
	while(n){
		m = bs_read(in, out->data+out->size, n);
		n -= m;
		out->size += m;
		if(m == 0){
			return 0;
		}
	}
	return out->size;
}

static __int64 pts_to_sample(__int64 pts, int rate)
{
	__int64 sec;
	__int64 mod;

	sec = pts/90000;
	mod = pts%90000;

	return (sec*rate+(mod*rate/90000));
}

static __int64 sample_to_pts(__int64 sample, int rate)
{
	__int64 sec;
	__int64 mod;

	sec = sample / rate;
	mod = sample % rate;

	return (sec*90000+(mod*90000/rate));
}

static int count_audio_frame(PES_PACKET *packet)
{
	int r;
	int n;
	unsigned int  sync;
	
	unsigned char *pos;
	unsigned char *last;

	LAYER2_HEADER head;

	n = get_pes_packet_data_length(packet);
	pos = packet->data+packet->size-n;
	last = pos+n-3;

	r = 0;
	while(pos<last){
		if( (pos[0] == 0xff) && ((pos[1] & 0xf0) == 0xf0) ){
			sync = ((unsigned int)(pos[0]) << 24) | ((unsigned int)(pos[1]) << 16) | ((unsigned int)(pos[2]) << 8) | ((unsigned int)(pos[3]));
			if(! parse_layer2_header(sync, &head)){
				pos += 1;
				continue;
			}
			if( pos+head.framesize >= last ){
				break;
			}
			if( (pos[head.framesize] == 0xff) && ((pos[head.framesize+1] & 0xf0) == 0xf0) ){
				r += 1;
				pos += head.framesize;
			}else{
				pos += 1;
			}
		}else{
			pos += 1;
		}
	}

	if(pos != last){
		r += 1;
	}

	return r;
}

static unsigned char *find_sync_current_ps_buffer(AUDIO_PS *ps)
{
	unsigned char *pos;
	unsigned char *last;
	unsigned int sync;

	LAYER2_HEADER h;

	last = ps->buffer.buffer+ps->buffer.size;
	while( (pos = mb_next_fff(&(ps->buffer))) != NULL ){
		if(pos+4 >= last ){
			break;
		}
		sync = (pos[0] << 24) | (pos[1] << 16) | (pos[2] << 8) | pos[3];
		if(parse_layer2_header(sync, &h)){
			if( pos+h.framesize == last ){
				return pos;
			}
			if( pos+h.framesize+1 >= last ){
				break;
			}
			if( (pos[h.framesize] == 0xff) && ((pos[h.framesize+1] & 0xf0) == 0xf0) ){
				return pos;
			}
		}
		ps->buffer.pos += 1;
	}

	return NULL;
}
