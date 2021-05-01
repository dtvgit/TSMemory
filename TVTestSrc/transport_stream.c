/******************************************************************
                       Transport Stream module
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "pes.h"
#include "001.h"
#include "registry.h"

#include "multi_file.h"

#define TRANSPORT_STREAM_C
#include "transport_stream.h"

typedef struct {

	MULTI_FILE     *mf;
	char           *path;

	unsigned char  *buffer;
	unsigned char  *current;
	int             buffer_size;
	int             buffer_max;
	
	int             pid;

	int             unit_size;
	int             offset;
	
	__int64         file_length;
	
	PES_STREAM_TYPE type;

	__int64         pos;

	PES_PACKET      packet;
	unsigned char  *packet_data;
	int             packet_rest;

} TRANSPORT_STREAM;

typedef struct {
	int sync;
	int transport_error_indicator;
	int payload_unit_start_indicator;
	int transport_priority;
	int pid;
	int transport_scrambling_control;
	int adaptation_field_control;
	int continuity_counter;
} TS_HEADER;

typedef struct {
	int pid;
	int count;
} PID_CANDIDATE;

int ts_open(const char *filename, int stream_type);
int ts_close(int in);
int ts_read(int in, void *data, unsigned int count);
__int64 ts_seek(int in, __int64 offset, int origin);
__int64 ts_tell(int in);

static int check_transport_stream(TRANSPORT_STREAM *ts);
static int reserve_buffer(TRANSPORT_STREAM *ts);
static void count_file_size(TRANSPORT_STREAM *ts);
static int fill_buffer(TRANSPORT_STREAM *ts);
static unsigned char *read_unit(TRANSPORT_STREAM *ts);
static void go_next_unit(TRANSPORT_STREAM *ts);
static void go_prev_unit(TRANSPORT_STREAM *ts);
static __int64 ts_seek_raw(TRANSPORT_STREAM *ts, __int64 offset, int origin);
static __int64 ts_tell_raw(TRANSPORT_STREAM *ts);
static int select_stream(TRANSPORT_STREAM *ts, __int64 offset);
static void extract_ts_header(unsigned char *buffer, TS_HEADER *header);
static int read_pes_packet(TRANSPORT_STREAM *ts);
static void extract_program_association_table(unsigned char *buffer, unsigned char *pid_type);
static int extract_program_map_table(unsigned char *buffer, unsigned char *pid_type, int type);
static int search_same_pid(PID_CANDIDATE *pc, int count, int pid);
static int add_section_data(PES_PACKET *dst, unsigned char *data, int size, TS_HEADER *hdr);
static int check_section_complete(PES_PACKET *dst);

int ts_open(const char *filename, int stream_type)
{
	int i,j,n;
	PID_CANDIDATE pid_candidate[8];
	TRANSPORT_STREAM *ts;

	ts = (TRANSPORT_STREAM *)calloc(1, sizeof(TRANSPORT_STREAM));
	if(ts == NULL){
		return -1;
	}

	n = strlen(filename)+1;
	ts->path = (char *)malloc(n);
	if(ts->path == NULL){
		free(ts);
		return -1;
	}
	memcpy(ts->path, filename, n);

	init_pes_packet(&ts->packet);

	ts->mf = open_multi_file(filename);
	if(ts->mf == NULL){
		free(ts->path);
		free(ts);
		return -1;
	}
	
	if(!check_transport_stream(ts)){
		ts->mf->close(ts->mf);
		free(ts->path);
		free(ts);
		return -1;
	}

	if(!reserve_buffer(ts)){
		ts->mf->close(ts->mf);
		free(ts->path);
		free(ts);
		return -1;
	}

	count_file_size(ts);

	fill_buffer(ts);

	memset(pid_candidate, 0, sizeof(pid_candidate));
	n = 0;
	
	ts->type.type = stream_type;
	for(i=0;i<8;i++){
		__int64 offset = i * ts->unit_size * (ts->file_length / (ts->unit_size * 8));
		if(!select_stream(ts, offset)){ /* failed to find stream */
			continue;
		}
		j = search_same_pid(pid_candidate, n, ts->pid);
		pid_candidate[j].pid = ts->pid;
		pid_candidate[j].count += 1;
		if (j >= n) {
			n += 1;
		}
	}

	ts->pid = 0;
	for(i=0;i<8;i++){
		if(pid_candidate[i].count >= 4){
			ts->pid = pid_candidate[i].pid;
			break;
		}
		if(pid_candidate[i].count == 0){
			break;
		}
	}

	if (ts->pid == 0) {
		ts_close((int)ts);
		return -1;
	}
	
	ts_seek((int)ts, 0, SEEK_SET);

	return (int)ts;
}

int ts_close(int in)
{
	TRANSPORT_STREAM *ts;

	ts = (TRANSPORT_STREAM *)in;

	if(in == -1){
		return 0;
	}
	
	if(ts == NULL){
		return 0;
	}

	if(ts->path){
		free(ts->path);
		ts->path = NULL;
	}

	if(ts->buffer){
		free(ts->buffer);
		ts->buffer = NULL;
	}

	if(ts->mf){
		ts->mf->close(ts->mf);
		ts->mf = NULL;
	}

	release_pes_packet(&ts->packet);

	free(ts);

	return 1;
}

int ts_read(int in, void *data, unsigned int count)
{
	int r;
	
	TRANSPORT_STREAM *ts;
	PES_STREAM_TYPE type;

	ts = (TRANSPORT_STREAM *)in;

	if(ts->packet_rest){
		if(ts->packet_rest <= count){
			r = ts->packet_rest;
		} else {
			r = count;
		}
		memcpy(data, ts->packet_data, r);
		ts->packet_data += r;
		ts->packet_rest -= r;
			
		return r;
	}

	r = 0;
	while(read_pes_packet(ts)){
		if(!extract_pes_stream_type(&ts->packet, &type)){
			continue;
		}
		if( (type.type == ts->type.type) && (type.id == ts->type.id) ){
			ts->packet_rest = get_pes_packet_data_length(&ts->packet);
			ts->packet_data = ref_pes_packet_data(&ts->packet);
			if(ts->packet_rest <= count){
				r = ts->packet_rest;
			} else {
				r = count;
			}
			memcpy(data, ts->packet_data, r);
			ts->packet_data += r;
			ts->packet_rest -= r;
			break;
		}
	}

	return r;
}

__int64 ts_seek(int in, __int64 offset, int origin)
{
	__int64           n;
	int               start_indicator;
	unsigned char    *p;
	TRANSPORT_STREAM *ts;
	TS_HEADER         header;
	PES_STREAM_TYPE   type;
	
	ts = (TRANSPORT_STREAM *)in;

	offset = ts_seek_raw(ts, offset, origin);
	n = offset % ts->unit_size;
	n = ts_seek_raw(ts, -n, SEEK_CUR);
	
	ts->pos = n;
	ts->packet_data = NULL;
	ts->packet_rest = 0;
	
	start_indicator = 0;
	while( (p = read_unit(ts)) != NULL ){
		extract_ts_header(p, &header);
		if(ts->pid == header.pid){
			if(start_indicator){
				break;
			}
			if(header.payload_unit_start_indicator){
				start_indicator = 1;
			}
		}
		go_prev_unit(ts);
		if(ts_tell_raw(ts) == 0){
			break;
		}
	}

	start_indicator = 0;
	while( read_pes_packet(ts) ){
		if(!extract_pes_stream_type(&ts->packet, &type)){
			continue;
		}
		if( (type.type == ts->type.type) && (type.id == ts->type.id) ){
			n = offset - ts->pos;
			if(n < 0){
				n = 0;
			}
			ts->packet_rest = get_pes_packet_data_length(&ts->packet);
			ts->packet_data = ref_pes_packet_data(&ts->packet);
			if(n < ts->packet_rest){
				ts->packet_rest -= (int)n;
				ts->packet_data += (int)n;
				break;
			} else {
				ts->packet_data += ts->packet_rest;
				ts->packet_rest = 0;
			}
		}
	}

	return ts_tell(in);		
}

__int64 ts_tell(int in)
{
	__int64           r;
	TRANSPORT_STREAM *ts;

	ts = (TRANSPORT_STREAM *)in;

	r = ts->pos + get_pes_packet_data_length(&ts->packet) - ts->packet_rest;

	return r;
}

static int check_transport_stream(TRANSPORT_STREAM *ts)
{
	int i,n;
	unsigned char buffer[6144];

	ts->mf->seek(ts->mf, 0, SEEK_SET);
	n = ts->mf->read(ts->mf, buffer, sizeof(buffer));
	ts->mf->seek(ts->mf, 0, SEEK_SET);
	
	for(i=0;i<n;i+=188){
		if(buffer[i] != 0x47){
			break;
		}
	}

	if(i>=n){
		ts->unit_size = 188;
		ts->offset = 0;
		return 1;
	}

	for(i=0;i<n;i+=192){
		if(buffer[i+4] != 0x47){
			break;
		}
	}

	if(i>=n){
		ts->unit_size = 192;
		ts->offset = 4;
		return 1;
	}

	for(i=0;i<n;i+=204){
		if(buffer[i] != 0x47){
			break;
		}
	}
	
	if(i>=n){
		ts->unit_size = 204;
		ts->offset = 0;
		return 1;
	}

	return 0;
}

static int reserve_buffer(TRANSPORT_STREAM *ts)
{
	int gcd,lcm;
	int m,n;
	int min = 512*1024;

	m = ts->unit_size;
	n = 4096;

	while(m!=n){
		if(m>n){
			m = m-n;
		}else{
			n = n-m;
		}
	}

	gcd = m;
	lcm = 4096/gcd*ts->unit_size;
	ts->buffer_max = lcm;
	while(ts->buffer_max < min){
		ts->buffer_max += lcm;
	}

	ts->buffer = (unsigned char *)malloc(ts->buffer_max);
	if(ts->buffer == NULL){
		return 0;
	}

	return 1;
}

static void count_file_size(TRANSPORT_STREAM *ts)
{
	__int64 set;
	__int64 end;

	end = ts->mf->seek(ts->mf, 0, SEEK_END);
	set = ts->mf->seek(ts->mf, 0, SEEK_SET);

	ts->file_length = end-set;
}

static int fill_buffer(TRANSPORT_STREAM *ts)
{
	ts->buffer_size = ts->mf->read(ts->mf, ts->buffer, ts->buffer_max);
	ts->current = ts->buffer;
	return ts->buffer_size;
}

static unsigned char *read_unit(TRANSPORT_STREAM *ts)
{
	if( (ts->current-ts->buffer) >= ts->buffer_size ){
		return NULL;
	}
	
	return ts->current + ts->offset;
}

static void go_next_unit(TRANSPORT_STREAM *ts)
{
	ts->current += ts->unit_size;

	if( (ts->current-ts->buffer) >= ts->buffer_size ){
		fill_buffer(ts);
	}
}

static void go_prev_unit(TRANSPORT_STREAM *ts)
{
	ts->current -= ts->unit_size;
	ts_seek_raw(ts, 0, SEEK_CUR);
}

static __int64 ts_seek_raw(TRANSPORT_STREAM *ts, __int64 offset, int origin)
{
	__int64 set,end;
	__int64 newpos;
	
	end = ts->mf->tell(ts->mf);
	set = end - ts->buffer_size;

	switch(origin){
	case SEEK_SET:
		newpos = offset;
		break;
	case SEEK_CUR:
		newpos = set + (ts->current - ts->buffer) + offset;
		break;
	case SEEK_END:
		newpos = ts->file_length + offset;
		break;
	default:
		newpos = offset;
		break;
	}

	if(newpos < 0){
		newpos = 0;
	}else if(newpos > ts->file_length){
		newpos = ts->file_length;
	}

	if( (newpos < set) || (end <= newpos) ){
		set = newpos / ts->buffer_max * ts->buffer_max;
		ts->mf->seek(ts->mf, set, SEEK_SET);
		fill_buffer(ts);
	}

	ts->current = ts->buffer + newpos - set;

	return newpos;
}

static __int64 ts_tell_raw(TRANSPORT_STREAM *ts)
{
	__int64 set;

	set = ts->mf->tell(ts->mf) - ts->buffer_size;

	return set + (ts->current - ts->buffer);
}

static int select_stream(TRANSPORT_STREAM *ts, __int64 offset)
{
	int r;
	int i,n,size;

	__int64 limit;
	
	unsigned char *p,*data;
	TS_HEADER header;
	PES_PACKET *packets[8192];
	unsigned char pid_type[8192];
	PES_STREAM_TYPE type;
	
	r = 0;

	memset(packets, 0, sizeof(packets));
	memset(pid_type, 0, sizeof(pid_type));
	
	limit = get_filecheck_limit();
	limit += offset;

	ts_seek_raw(ts, offset, SEEK_SET);
	packets[0x0000] = (PES_PACKET *)malloc(sizeof(PES_PACKET));
	init_pes_packet(packets[0x0000]);

	/* search Program Association Table */
	while( (p = read_unit(ts)) != NULL ){
		extract_ts_header(p, &header);
		if(header.pid == 0x0000){
			data = p+4;
			size = 188-4;
			if(header.adaptation_field_control & 2){
				n = data[0] + 1;
				size -= n;
				data += n;
			}
			add_section_data(packets[0x0000], data, size, &header);
			if (check_section_complete(packets[0x0000])) {
				extract_program_association_table(packets[0x0000]->data, pid_type);
				if(pid_type[0] == TS_STREAM_TYPE_PAT){
					break;
				}
			}
		}
		go_next_unit(ts);
		if(ts_tell_raw(ts) > limit){
			break;
		}
	}

	if(pid_type[0] != TS_STREAM_TYPE_PAT){
		ts_seek((int)ts, offset, SEEK_SET);
		goto NOT_FOUND_PAT_PMT;
	}

	/* search Program Map Table */
	while( (p = read_unit(ts)) != NULL ){
		extract_ts_header(p, &header);
		if(pid_type[header.pid] == TS_STREAM_TYPE_PMT){
			data = p+4;
			size = 188-4;
			if(header.adaptation_field_control & 2){
				n = data[0] + 1;
				size -= n;
				data += n;
			}
			if (packets[header.pid] == NULL) {
				packets[header.pid] = (PES_PACKET *)malloc(sizeof(PES_PACKET));
				init_pes_packet(packets[header.pid]);
			}
			add_section_data(packets[header.pid], data, size, &header);
			if (check_section_complete(packets[header.pid])) {
				ts->pid = extract_program_map_table(packets[header.pid]->data, pid_type, ts->type.type);
				if(ts->pid != 0){
					r = 1;
					goto SELECT_STREAM_LAST;
				}
			}
		}
		go_next_unit(ts);
		if(ts_tell_raw(ts) > limit){
			break;
		}
	}
	ts_seek((int)ts, 0, SEEK_SET);

	/* no Program Map Table */
NOT_FOUND_PAT_PMT:
	while( (p = read_unit(ts)) != NULL ){
		extract_ts_header(p, &header);
		if( (header.pid < 0x10) || (header.pid == 0x1fff) ){
		}else if(header.adaptation_field_control & 1){
			if(packets[header.pid] == NULL){
				packets[header.pid] = (PES_PACKET *)malloc(sizeof(PES_PACKET));
				init_pes_packet(packets[header.pid]);
			}
			data = p+4;
			if(header.adaptation_field_control & 2){
				data += data[0] + 1;
			}
			n = 188 - (data-p);
			append_pes_packet_data(packets[header.pid], data, n);
			extract_pes_stream_type(packets[header.pid], &type);
			if(type.type == ts->type.type){
				ts->type.id = type.id;
				ts->pid = header.pid;
				r = 1;
				//break;
			}else if(type.type != PES_STREAM_TYPE_UNKNOWN){
				release_pes_packet(packets[header.pid]);
			}
		}
		go_next_unit(ts);
		if(ts_tell_raw(ts) > limit){
			break;
		}
	}
SELECT_STREAM_LAST:
	for(i=0;i<8192;i++){
		if(packets[i]){
			release_pes_packet(packets[i]);
			free(packets[i]);
		}
	}

	return r;
}

static void extract_ts_header(unsigned char *buffer, TS_HEADER *header)
{
	header->sync = buffer[0] & 0xff;
	header->transport_error_indicator = (buffer[1] >> 7) & 1;
	header->payload_unit_start_indicator = (buffer[1] >> 6) & 1;
	header->transport_priority = (buffer[1] >> 5) & 1;
	header->pid = (((buffer[1] & 0x1f)) << 8) + (buffer[2] & 0xff);
	header->transport_scrambling_control = (buffer[3] >> 6) & 3;
	header->adaptation_field_control = (buffer[3] >> 4) & 3;
	header->continuity_counter = (buffer[3] & 0xf);
	if(header->pid == 0x101){
		header->sync = 0x47;
	}
}

static int read_pes_packet(TRANSPORT_STREAM *ts)
{
	PES_PACKET *out;
	TS_HEADER header;
	unsigned char *p,*data;
	int n;
	int start_indicator;

	out = &ts->packet;
	start_indicator = 0;

	release_pes_packet(out);
	
	while( (p = read_unit(ts)) != NULL ){
		extract_ts_header(p, &header);
		if(header.pid != ts->pid){
			go_next_unit(ts);
			continue;
		}
		if(header.payload_unit_start_indicator && (start_indicator == 0)){
			ts->pos = ts_tell_raw(ts);
			start_indicator = 1;
		}
		if(start_indicator && (header.adaptation_field_control & 1)){
			data = p+4;
			if(header.adaptation_field_control & 2){
				data += data[0] + 1;
			}
			n = 188 - (data-p);
			if(append_pes_packet_data(out, data, n) < n){
				return 1;
			}
		}
		go_next_unit(ts);
	}

	return 0;
}

static void extract_program_association_table(unsigned char *buffer, unsigned char *pid_type)
{
	unsigned char *p;

	int section_length;
	int transport_stream_id;
	int version_number;
	int current_next_indicator;
	int section_number;
	int last_section_number;
	int program_number;
	int id;
	
	p = buffer;
	if(p[0] != 0x00){ /* not Program Association table_id */
		return;
	}
	section_length = ((p[1] & 0xf) << 8) + p[2];
	transport_stream_id = (p[3] << 8) + p[4];
	version_number = (p[5] & 0x3e) >> 1;
	current_next_indicator = p[5] & 1;
	section_number = p[6];
	last_section_number = p[7];

	p += 8;
	section_length -= 5;

	while(section_length > 4){
		program_number = (p[0] << 8) + p[1];
		id = ((p[2] & 0x1f) << 8) + p[3];
		if(program_number == 0){
			pid_type[id] = TS_STREAM_TYPE_NIT;
		}else{
			pid_type[id] = TS_STREAM_TYPE_PMT;
		}
		p += 4;
		section_length -= 4;
	}

	pid_type[0] = TS_STREAM_TYPE_PAT;
}

static int extract_program_map_table(unsigned char *buffer, unsigned char *pid_type, int type)
{
	unsigned char *p;

	int r;

	int section_length;
	int program_number;
	int version_number;
	int current_next_indicator;
	int section_number;
	int last_section_number;
	int PCR_PID;
	int program_info_length;

	int stream_type;
	int elementary_PID;
	int es_info_length;

	r = 0;
	
	p = buffer;
	if(p[0] != 0x02){ /* not Program Map table_id */
		return r;
	}
	section_length = ((p[1] & 0xf) << 8) + p[2];
	program_number = (p[3] << 8) + p[4];
	version_number = (p[5] & 0x3e) >> 1;
	current_next_indicator = p[5] & 1;
	section_number = p[6];
	last_section_number = p[7];
	PCR_PID = ((p[8] & 0x1f) << 8) + p[9];
	program_info_length = ((p[10] & 0xf) << 8) + p[11];
	
	p += 12 + program_info_length;
	section_length -= 9 + program_info_length;

	while(section_length > 4){
		stream_type = p[0];
		elementary_PID = ((p[1] & 0x1f) << 8) + p[2];
		es_info_length = ((p[3] & 0x0f) << 8) + p[4];
		switch(stream_type){
		case 0x01:
			pid_type[elementary_PID] = TS_STREAM_TYPE_V11172_2;
			if( (type == PES_STREAM_TYPE_VIDEO) && (r == 0) ){
				r = elementary_PID;
			}
			break;
		case 0x02:
			pid_type[elementary_PID] = TS_STREAM_TYPE_V13818_2;
			if( (type == PES_STREAM_TYPE_VIDEO) && (r == 0) ){
				r = elementary_PID;
			}
			break;
		case 0x03:
			pid_type[elementary_PID] = TS_STREAM_TYPE_A11172_3;
			break;
		case 0x04:
			pid_type[elementary_PID] = TS_STREAM_TYPE_A13818_3;
			break;
		case 0x0f:
			pid_type[elementary_PID] = TS_STREAM_TYPE_A13818_7;
			break;
		case 0x10:
			pid_type[elementary_PID] = TS_STREAM_TYPE_V14496_2;
			break;
		}			
		p += 5 + es_info_length;
		section_length -= es_info_length;
	}

	return r;
}

static int search_same_pid(PID_CANDIDATE *pc, int count, int pid)
{
	int i;
	for(i=0;i<count;i++){
		if (pc[i].pid == pid) {
			return i;
		}
	}
	return i;
}

static int add_section_data(PES_PACKET *dst, unsigned char *data, int size, TS_HEADER *hdr)
{
	if (dst->data == NULL) {
		dst->capacity = 4*1024;
		dst->data = (unsigned char *)calloc(1, dst->capacity);
		if (dst->data == NULL) {
			return 0;
		}
	}

	if (dst->size > 0) {
		if (hdr->payload_unit_start_indicator) {
			int n = data[0]; // pointer field
			memcpy(dst->data + dst->size, data+1, n);
			dst->size += n;
		} else {
			memcpy(dst->data + dst->size, data, size);
			dst->size += size;
			data += size;
			size = 0;
		}

		if ((dst->size > 2) && (dst->data_length == 0)) {
			unsigned char *p = dst->data;
			dst->data_length = ((p[1] << 8) | p[2]) & 0xfff;
		}

		if (check_section_complete(dst)) {
			return 1;
		}
	}
			
	if (hdr->payload_unit_start_indicator) {
		int n = data[0] + 1; // pointer field
		data += n;
		size -= n;
		memcpy(dst->data, data, size);
		dst->size = size;
		dst->data_length = 0;

		if (dst->size > 2) {
			unsigned char *p = dst->data;
			dst->data_length = ((p[1] << 8) | p[2]) & 0xfff;
		}
	}

	return 1;
}

static int check_section_complete(PES_PACKET *ptr)
{
	if ((ptr->data_length > 0) && (ptr->data_length <= ptr->size)) {
		return 1;
	}

	return 0;
}

