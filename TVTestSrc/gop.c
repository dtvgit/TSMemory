/*******************************************************************
                            GOP module
 *******************************************************************/

#include <io.h>
#include "sequence_header.h"

#include "gop.h"

static int check_gop_timecode(READ_GOP_PARAMETER *in, __int64 frame_count);

int next_gop(VIDEO_STREAM *p)
{
	unsigned int code;

	while(vs_next_start_code(p)){
		code = vs_read_bits(p, 32);
		if(code == 0x1B8){
			return 1;
		}
		vs_erase_bits(p, 32);
	}

	return 0;
}

int last_gop(VIDEO_STREAM *p)
{
	__int64 n,size;

	n = 0;
	size = 128 * 1024;
	
	while(n == 0){
		
		if(p->file_length < size){
			video_stream_seek(p, 0, SEEK_SET);
		}else{
			video_stream_seek(p, - size, SEEK_END);
		}

		while(next_gop(p)){
			n = video_stream_tell(p);
			vs_erase_bits(p, 32);
		}

		if(p->file_length < size){
			video_stream_seek(p, n, SEEK_SET);
			return 0;
		}
		
		size *= 4;
	}

	video_stream_seek(p, n, SEEK_SET);
	
	return 1;
}

READ_GOP_PARAMETER *new_read_gop_parameter(VIDEO_STREAM *stream, SEQUENCE_HEADER *seq, READ_PICTURE_HEADER_OPTION *pic_opt, int field_order)
{
	READ_GOP_PARAMETER *r;

	static const int rate[16] = {
		 0, 24000, 24, 25, 30000, 30, 50, 60000,
		60,     0,  0,  0,     0,  0,  0,     0,
	};

	static const int scale[16] = {
		 1,  1001,  1,  1,  1001,  1,  1,  1001,
		 1,     1,  1,  1,     1,  1,  1,     1,
	};

	r = (READ_GOP_PARAMETER *)calloc(1, sizeof(READ_GOP_PARAMETER));
	if(r == NULL){
		return NULL;
	}

	r->p = stream;

	r->rate = rate[seq->picture_rate];
	r->scale = scale[seq->picture_rate];

	if(seq->has_sequence_extension){
		r->rate *= (seq->se.frame_rate_ext_n + 1);
		r->scale *= (seq->se.frame_rate_ext_d + 1);
	}

	r->start_frame = 0;

	r->field_order = field_order;
	
	r->pic_opt = pic_opt;

	return r;
}

void delete_read_gop_parameter(void *p)
{
	if(p != NULL){
		free(p);
	}
}	

__int64 read_gop(READ_GOP_PARAMETER *in, GOP *out)
{
	int n;
	int code;

	int closed_gop;
	int broken_link;

	int prev_temporal_reference;

	int temporal_count;
	int count_of_frames[5];
	int step;
	
	int frame_rate;
	TIMECODE t[2];

	PICTURE_HEADER pic;

	__int64 seq_offset;

	memset(out, 0, sizeof(GOP));

	seq_offset = 0;
	code = 0;
	prev_temporal_reference = -1;

	while(vs_next_start_code(in->p)){
		code = vs_read_bits(in->p, 32);
		if(code == 0x1b3){
			seq_offset = video_stream_tell(in->p);
		}else if(code == 0x1b8){
			break;
		}else if(code == 0x100){
			seq_offset = 0;
		}
		vs_erase_bits(in->p, 32);
	}
	if(code != 0x1b8){
		return 0;
	}

	if(seq_offset){
		out->offset = seq_offset;
	}else{
		out->offset = video_stream_tell(in->p);
	}
	vs_erase_bits(in->p, 32);
	
	read_timecode(in->p, t);
	closed_gop = vs_get_bits(in->p, 1);
	broken_link = vs_get_bits(in->p, 1);

	/****************************************************************
	* next procedure steps
	*
	* step_0 - check i_picture existance
	* step_1 - count b_pictures whats is shown before i_picture
	* step_2 - count pictures for next_gop
	* step_3 - check i_picture existance
	* step_4 - count next b_pictures
	****************************************************************/

	step = 0;
	code = 0;
	memset(count_of_frames, 0, sizeof(count_of_frames));

	while( (step < 5) && vs_next_start_code(in->p) ){
		
		code = vs_get_bits(in->p, 32);
		
		if(code == 0x100){ /* picture */

			if(! read_picture_header(in->p, &pic, in->pic_opt) ){
				return 0; /* ERROR - read_picture_header is faild */
			}

			if (pic.temporal_reference == prev_temporal_reference) {
				/* frame is doubling - skip this frame. */
				continue;
			}

			prev_temporal_reference = pic.temporal_reference;

			temporal_count = 1;

			if( pic.has_picture_coding_extension ){ /* MPEG-2 VIDEO */
				
				if( pic.pc.picture_structure != 3 ){ /* field picture */
					if(! skip_2nd_field(in->p, in->pic_opt, &pic) ){
						return 0; /* ERROR - skip_2nd_field is faild */
					}
				}

				if( pic.pc.repeat_first_field && (pic.pc.top_field_first == in->field_order) ){
					temporal_count += 1;
				}
			}

			switch(step){
			case 0:
				if(pic.picture_coding_type != 1){
					return 0; /* ERROR - gop start picture is not I picture */
				}
				count_of_frames[0] += temporal_count;
				step = 1;
				break;
			case 1:
				if(pic.picture_coding_type == 1){
					return 0; /* ERROR - gop has multiple I picture */
				}
				if(pic.picture_coding_type == 2){
					step = 2;
					broken_link = 0;
					closed_gop = 0;
				}
				if(! broken_link){
					if(closed_gop){
						count_of_frames[step-1] += temporal_count;
					}else{
						count_of_frames[step] += temporal_count;
					}
				}
				break;
			case 2:
				if(pic.picture_coding_type == 1){
					return 0; /* ERROR - gop has multiple I picture */
				}
				count_of_frames[step] += temporal_count;
				break;
			case 3:
				if(pic.picture_coding_type != 1){
					return 0; /* ERROR - gop start picture is not I picture */
				}
				count_of_frames[step] += temporal_count;
				step = 4;
				break;
			case 4:
				if(pic.picture_coding_type == 1){
					return 0; /* ERROR - gop has multiple I picture */
				}
				if(pic.picture_coding_type == 2){
					step = 5;
					break;
				}
				if(! broken_link){
					count_of_frames[step] += temporal_count;
				}
				break;
			default:
				return 0; /* ERROR - unknown step */
			}
			
		}else if(code == 0x1b8){ /* GOP */
			if(step == 4){ /* next gop has no P picture */
				step = 5;
				break;
			}

			read_timecode(in->p, t+1);
			closed_gop = vs_get_bits(in->p, 1);
			broken_link = vs_get_bits(in->p, 1);
			prev_temporal_reference = -1;

			if(step == 0){
				return 0; /* ERROR - GOP has no picture */
			}

			if(closed_gop){/* closed_gop B pictures include next GOP */
				step = 5;
				break;
			}
			step = 3;
		}

	}

	/* check GOP timecode sequence */
	frame_rate = (in->rate + in->scale - 1 ) / in->scale;
	if(step == 5){
		n = (int)(timecode2frame(t+1, frame_rate) - timecode2frame(t, frame_rate));
		if(n != count_of_frames[0] + count_of_frames[1] + count_of_frames[2]){
			return 0; /* ERROR - GOP timecode is not sequential */
		}
	}else{ /* last gop */
		if(code != 0x1b7){ /* not sequence end code */
			count_of_frames[2] -= 2;
		}
	}

	/* set output data */
	out->start_frame = timecode2frame(t, frame_rate);
	out->start_frame -= in->start_frame;
	out->start_frame += count_of_frames[1];
	
	out->frame_count = count_of_frames[0] + count_of_frames[2] + count_of_frames[4];

	return out->frame_count;
}

int skip_2nd_field(VIDEO_STREAM *in, READ_PICTURE_HEADER_OPTION *opt, PICTURE_HEADER *pic_1st)
{
	int code;
	__int64 offset;
	
	PICTURE_HEADER pic;

	while(vs_next_start_code(in)){
		offset = video_stream_tell(in);
		code = vs_get_bits(in, 32);
		if(code == 0x100){
			read_picture_header(in, &pic, opt);
			if (pic.has_picture_coding_extension == 0) {
				return 0;
			}
			if ((pic.pc.picture_structure == 3) ||
			    (pic.pc.picture_structure == pic_1st->pc.picture_structure)) {
				/* broken stream */
				if (pic.temporal_reference == pic_1st->temporal_reference) {
					/* 1st field is doubling */
					continue;
				} else {
					/* 2nd field is droped */
					video_stream_seek(in, offset, SEEK_SET);
				}
			}
			return 1;
		}
	}

	return 0;
}

__int64 count_frame(READ_GOP_PARAMETER *p)
{
	__int64 r;
	GOP g;

	video_stream_seek(p->p, 0, SEEK_SET);

	p->start_frame = 0;
	
	if(! read_gop(p, &g)){
		return -1;
	}

	p->start_frame = g.start_frame;
	
	if(video_stream_tell(p->p) == p->p->file_length){
		r = g.frame_count;
	}else{
		last_gop(p->p);
		read_gop(p, &g);
		r = g.start_frame + g.frame_count;
		
		if(!check_gop_timecode(p, r)){
			return -1;
		}
	}

	return r;
}	

GOP find_gop_with_timecode(void *p, __int64 frame)
{
	__int64 first, last, i, n;

	GOP r;

	READ_GOP_PARAMETER *gp;

	gp = (READ_GOP_PARAMETER *)p;

	first = 0;
	last = gp->p->file_length;

	while(1){
		i = first + (last - first)/2;
		
		if(i == first){
			break;
		}
		
		video_stream_seek(gp->p, i, SEEK_SET);

		n = read_gop(gp, &r);

		if( n != r.frame_count ){
			break;
		}
		
		if(n == 0){
			last = i;
			continue;
		}
		
		if(r.start_frame > frame){
			last = i;
		}else if(r.start_frame + r.frame_count > frame){
			return r;
		}else{
			first = r.offset;
		}
	}
		
	r.offset = 0;
	r.start_frame = 0;
	r.frame_count = 0;
	
	return r;		
}

static int check_gop_timecode(READ_GOP_PARAMETER *in, __int64 frame_count)
{
	__int64 step;
	__int64 s[9];
	
	int i;
	
	GOP gop;
	
	step = video_stream_seek(in->p, 0, SEEK_END) / 8;
	s[0] = in->start_frame;
	for(i=0;i<8;i++){
		video_stream_seek(in->p, step*i, SEEK_SET);
		read_gop(in, &gop);
		s[i+1] = gop.start_frame;
		if(s[i] > s[i+1]){
			return 0;
		}
		if(s[i+1] > frame_count){
			return 0;
		}
	}

	return 1;
}

