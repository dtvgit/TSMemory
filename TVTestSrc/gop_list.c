/*******************************************************************
                         GOP list module
 *******************************************************************/

#include "endian.h"
#include "gl_dialog.h"

#include "gop_list.h"

typedef struct {
	int     index;
	int     sh_index;
	__int64 start;
	__int64 count;

	__int64 offset;

	void   *prev;
	void   *next;
} GOP_ENTRY;

typedef struct {
	int     index;
	SEQUENCE_HEADER sh;

	void *prev;
	void *next;
} SEQUENCE_ENTRY;

static const char GL_TAG[32] = {
	'G', 'O', 'P', '_', 'L', 'I', 'S', 'T',
	0, 0, 0, 0, 0, 0, 0, 0, 
	'0', '0', '0', '0', '0', '0', '0', '3',
	0, 0, 0, 0, 0, 0, 0, 0, 
};

static void release_gop_entries(GOP_ENTRY *last);
static void release_sequence_entries(SEQUENCE_ENTRY *last);

static int save_sequence_header(SEQUENCE_HEADER *sh, FILE *fp);
static int load_sequence_header(SEQUENCE_HEADER *sh, FILE *fp);

GOP_LIST *new_gop_list(VIDEO_STREAM *in, READ_PICTURE_HEADER_OPTION *pic_opt, int field_order, int dialog_mode)
{
	GOP_LIST *r;
	GL_DIALOG *dlg;

	int code;
	
	int broken_link;
	int closed_gop;
	int temporal_count;
	int intra_flag;

	int prev_temporal_reference;

	int i;

	__int64 offset;
	__int64 frame;

	__int64 gop_offset;

	PICTURE_HEADER pic;
	SEQUENCE_HEADER sh;
	
	GOP_ENTRY *p;
	GOP_ENTRY *c;

	SEQUENCE_ENTRY *sp;
	SEQUENCE_ENTRY *sc;

	/* stream rewind */
	video_stream_seek(in, 0, SEEK_SET);

	p = NULL;
	c = NULL;

	sp = NULL;
	sc = NULL;
	
	frame = 0;

	code = 0;
	
	intra_flag = 1;
	broken_link = 1;
	closed_gop = 0;

	gop_offset = -1;

	prev_temporal_reference = -1;

	memset(&pic, 0, sizeof(pic));

	dlg = create_gl_dialog(dialog_mode);

	/* 1st step - find initial I picture */
	while(vs_next_start_code(in)){

		if(dlg->is_cancel(dlg)){
			dlg->delete(dlg);
			return NULL;
		}
		dlg->update(dlg, (int)(video_stream_tell(in)*100/in->file_length));
		
		code = vs_read_bits(in, 32);
		if(code == 0x1b3){
			
			vs_erase_bits(in, 32);

			if(! read_sequence_header(in, &sh)){
				dlg->delete(dlg);
				return NULL;
			}
			sc = (SEQUENCE_ENTRY *)malloc(sizeof(SEQUENCE_ENTRY));
			sc->index = 0;
			sc->prev = NULL;
			memcpy(&(sc->sh), &sh, sizeof(SEQUENCE_HEADER));

		}else if(code == 0x1b8){

			gop_offset = video_stream_tell(in);

			vs_erase_bits(in, 32); // erase gop_start_code
			vs_erase_bits(in, 25); // erase timecode

			closed_gop = vs_get_bits(in, 1);
			broken_link = vs_get_bits(in, 1);

			prev_temporal_reference = -1;

		}else if(code == 0x100){
			
			offset = video_stream_tell(in);
			vs_erase_bits(in, 32);

			if(! read_picture_header(in, &pic, pic_opt)){
				dlg->delete(dlg);
				return NULL;
			}

			if (pic.temporal_reference == prev_temporal_reference) {
				/* frame is doubling - skip this frame. */
				continue;
			}

			prev_temporal_reference = pic.temporal_reference;
			
			temporal_count = 1;
			if(pic.has_picture_coding_extension){
				if(pic.pc.picture_structure != 3){
					if(! skip_2nd_field(in, pic_opt, &pic)){
						dlg->delete(dlg);
						return NULL;
					}
				}

				if(pic.pc.repeat_first_field && (pic.pc.top_field_first == field_order)){
					temporal_count += 1;
				}
			}

			if(pic.picture_coding_type == 1){
				c = (GOP_ENTRY *)malloc(sizeof(GOP_ENTRY));
				c->index = 0;
				c->sh_index = sc->index;
				c->start = 0;
				if(gop_offset >= 0){
					c->offset = gop_offset;
				}else{
					c->offset = offset;
				}
				c->prev = NULL;

				frame = temporal_count;

				break; /* goto 2nd step */
			}

			gop_offset = -1;
			closed_gop = 0;
			broken_link = 1;
		}else{
			vs_erase_bits(in, 32);
		}
	}

	if(closed_gop){
		broken_link = 0;
	}else{
		broken_link = 1;
	}

	/* 2nd step - check all rest pictures */
	while(vs_next_start_code(in)){
		
		if(dlg->is_cancel(dlg)){
			dlg->delete(dlg);
			release_sequence_entries(sc);
			release_gop_entries(c);
			return NULL;
		}
		dlg->update(dlg, (int)(video_stream_tell(in)*100/in->file_length));
		
		code = vs_read_bits(in, 32);
		if(code == 0x1b3){

			vs_erase_bits(in, 32);

			if(! read_sequence_header(in, &sh)){
				break;
			}

			if( memcmp(&(sc->sh), &sh, sizeof(SEQUENCE_HEADER)) ){
				sp = sc;
				sc = (SEQUENCE_ENTRY *)malloc(sizeof(SEQUENCE_ENTRY));
				if(sc == NULL){
					dlg->delete(dlg);
					release_sequence_entries(sp);
					release_gop_entries(c);
					return NULL;
				}
				sp->next = sc;
				sc->prev = sp;

				sc->index = sp->index + 1;
				memcpy(&(sc->sh), &sh, sizeof(SEQUENCE_HEADER));
			}

		}else if(code == 0x100){
			offset = video_stream_tell(in);
			vs_erase_bits(in, 32);

			if(! read_picture_header(in, &pic, pic_opt)){
				break;
			}

			if (pic.temporal_reference == prev_temporal_reference) {
				/* frame is doubling - skip this frame. */
				continue;
			}

			prev_temporal_reference = pic.temporal_reference;
			
			temporal_count = 1;
			if(pic.has_picture_coding_extension){
				if(pic.pc.picture_structure != 3){
					if(! skip_2nd_field(in, pic_opt, &pic)){
						break;
					}
				}

				if(pic.pc.repeat_first_field && (pic.pc.top_field_first == field_order) ){
					temporal_count += 1;
				}
			}

			switch(pic.picture_coding_type){
			case 1:
				if(intra_flag && p){
					p->count = c->start - p->start;
				}
				
				p = c;
				c = (GOP_ENTRY *)malloc(sizeof(GOP_ENTRY));
				if(c == NULL){
					dlg->delete(dlg);
					release_sequence_entries(sc);
					release_gop_entries(p);
					return NULL;
				}
				p->next = c;
				c->prev = p;
				
				c->index = p->index + 1;
				c->sh_index = sc->index;
				c->start = p->start + frame;
				if(gop_offset >= 0){
					c->offset = gop_offset;
				}else{
					c->offset = offset;
				}
				
				frame = temporal_count;
				intra_flag = 1;
				break;
			case 2:
				if(intra_flag && p){
					p->count = c->start - p->start;
				}
				frame += temporal_count;
				broken_link = 0;
				intra_flag = 0;
				break;
			case 3:
				if(intra_flag){
					if(broken_link){
						/* ignore this B picture */
						break;
					}
					if(closed_gop){
						frame += temporal_count;
					}else{
						c->start += temporal_count;
					}
				}else{
					frame += temporal_count;
				}
				break;
			}
			gop_offset = -1;
		}else if(code == 0x1b8){
			gop_offset = video_stream_tell(in);

			vs_erase_bits(in, 32); /* start_code */
			vs_erase_bits(in, 25); /* time_code */
			closed_gop = vs_get_bits(in, 1);
			broken_link = vs_get_bits(in, 1);
			if(closed_gop){
				broken_link = 0;
			}
			prev_temporal_reference = -1;
		}else{
			vs_erase_bits(in, 32);
		}
	}

	dlg->delete(dlg);

	if(code != 0x1b7){ /* not sequence end code */
		frame -= 2;
	}

	c->count = frame;
	if(p != NULL){
		p->count = c->start - p->start;
	}
	r = (GOP_LIST *)calloc(1, sizeof(GOP_LIST));
	if(r == NULL){
		release_sequence_entries(sc);
		release_gop_entries(c);
		return NULL;
	}
	
	r->stream_length = in->file_length;
	r->num_of_frame = c->start + frame;
	r->num_of_gop = c->index + 1;
	r->num_of_sh = sc->index + 1;

	/* convert SEQUENCE_ENTRY to GOP_LIST::sh */
	r->sh = (SEQUENCE_HEADER *)malloc(r->num_of_sh*sizeof(SEQUENCE_HEADER));
	if(r->sh == NULL){
		release_sequence_entries(sc);
		release_gop_entries(c);
		return NULL;
	}

	for(i=r->num_of_sh-1;sc;i--){
		memcpy(r->sh+i, &(sc->sh), sizeof(SEQUENCE_HEADER));
		sp = sc->prev;
		free(sc);
		sc = sp;
	}

	/* convert GOP_ENTRY to GOP_LIST::gop */
	r->gop = (GOP *)malloc((r->num_of_gop)*sizeof(GOP));
	if(r->gop == NULL){
		release_sequence_entries(sc);
		release_gop_entries(c);
		return NULL;
	}

	for(i=r->num_of_gop-1;c;i--){
		r->gop[i].offset = c->offset;
		r->gop[i].start_frame = c->start;
		r->gop[i].frame_count = c->count;
		r->gop[i].sh = r->sh + c->sh_index;
		p = (GOP_ENTRY *)c->prev;
		free(c);
		c = p;
	}

	if(r->gop[0].start_frame != 0){ // closed_gop
		r->gop[0].frame_count += r->gop[0].start_frame;
		r->gop[0].start_frame = 0;
	}
	
	return r;
}

void delete_gop_list(void *gop_list)
{
	GOP_LIST *w;

	w = (GOP_LIST *)gop_list;
	
	if(w != NULL){
		if(w->gop != NULL){
			free(w->gop);
		}
		if(w->sh != NULL){
			free(w->sh);
		}
		free(w);
	}
}

GOP find_gop_with_gop_list(void *p, __int64 frame)
{
	int first, last;
	int i;

	GOP_LIST *gl;
	
	static const GOP r = {
		0, 0, 0,
	};

	gl = (GOP_LIST *)p;
	
	if(frame < gl->gop[0].start_frame || frame > gl->num_of_frame-1){
		return r;
	}

	if(gl->gop[gl->num_of_gop-1].start_frame <= frame){
		return gl->gop[gl->num_of_gop-1];
	}

	first = 0;
	last = gl->num_of_gop - 1;

	i = last/2;
	
	while(1){
		if(gl->gop[i].start_frame <= frame && gl->gop[i+1].start_frame > frame){
			return gl->gop[i];
		}else if(gl->gop[i].start_frame < frame){
			first = i;
		}else{
			last = i;
		}
		i = first + (last - first)/2;
		if(first == last){
			return r;
		}
	}
}

int store_gop_list(GOP_LIST *in, char *filepath)
{
	FILE *out;

	int i;

	out = fopen(filepath, "wb");
	if(out == NULL){
		return 0;
	}

	if(fwrite(GL_TAG, 1, sizeof(GL_TAG), out) != sizeof(GL_TAG)){
		fclose(out);
		return 0;
	}

	if(! write_le_int64(in->stream_length, out) ){
		fclose(out);
		return 0;
	}
	
	if(! write_le_int64(in->num_of_frame, out) ){
		fclose(out);
		return 0;
	}

	if(! write_le_int32(in->num_of_gop, out) ){
		fclose(out);
		return 0;
	}

	if(! write_le_int32(in->num_of_sh, out) ){
		fclose(out);
		return 0;
	}

	for(i=0;i<in->num_of_gop;i++){
		if(! write_le_int64(in->gop[i].offset, out)){
			fclose(out);
			return 0;
		}
		if(! write_le_int64(in->gop[i].start_frame, out)){
			fclose(out);
			return 0;
		}
		if(! write_le_int64(in->gop[i].frame_count, out)){
			fclose(out);
			return 0;
		}
		if(! write_le_int32((int)(in->gop[i].sh - in->sh), out)){
			fclose(out);
			return 0;
		}
	}

	for(i=0;i<in->num_of_sh;i++){
		if(! save_sequence_header(in->sh+i, out)){
			fclose(out);
			return 0;
		}
	}

	fclose(out);

	return 1;
}

GOP_LIST *load_gop_list(char *filepath)
{
	int offset;
	GOP_LIST *r;
	FILE *in;

	char buf[sizeof(GL_TAG)];
	
	int i;

	in = fopen(filepath, "rb");
	if(in == NULL){
		return NULL;
	}

	i = fread(buf, 1, sizeof(buf), in);
	if(i != sizeof(buf)){
		fclose(in);
		return NULL;
	}
	
	if(memcmp(buf, GL_TAG, sizeof(buf)) != 0){
		fclose(in);
		return NULL;
	}
	
	r = (GOP_LIST *)calloc(1, sizeof(GOP_LIST));
	if(r == NULL){
		fclose(in);
		return NULL;
	}

	if(! read_le_int64(in, &(r->stream_length)) ){
		fclose(in);
		free(r);
		return NULL;
	}

	if(! read_le_int64(in, &(r->num_of_frame)) ){
		fclose(in);
		free(r);
		return NULL;
	}

	if(! read_le_int32(in, &(r->num_of_gop)) ){
		fclose(in);
		free(r);
		return NULL;
	}

	if(! read_le_int32(in, &(r->num_of_sh)) ){
		fclose(in);
		free(r);
		return NULL;
	}

	if(r->num_of_gop < 1){
		fclose(in);
		free(r);
		return NULL;
	}

	if(r->num_of_sh < 1){
		fclose(in);
		free(r);
		return NULL;
	}

	r->gop = (GOP *)calloc(r->num_of_gop, sizeof(GOP));
	if(r->gop == NULL){
		fclose(in);
		free(r);
		return NULL;
	}

	r->sh = (SEQUENCE_HEADER *)calloc(r->num_of_sh, sizeof(SEQUENCE_HEADER));
	if(r->sh == NULL){
		fclose(in);
		free(r->gop);
		free(r);
		return NULL;
	}

	for(i=0;i<r->num_of_gop;i++){
		if(! read_le_int64(in, &(r->gop[i].offset)) ){
			fclose(in);
			free(r->gop);
			free(r->sh);
			free(r);
			return NULL;
		}
		
		if(! read_le_int64(in, &(r->gop[i].start_frame)) ){
			fclose(in);
			free(r->gop);
			free(r->sh);
			free(r);
			return NULL;
		}
		
		if(! read_le_int64(in, &(r->gop[i].frame_count)) ){
			fclose(in);
			free(r->gop);
			free(r->sh);
			free(r);
			return NULL;
		}

		if(! read_le_int32(in, &offset) ){
			fclose(in);
			free(r->gop);
			free(r->sh);
			free(r);
			return NULL;
		}
		r->gop[i].sh = r->sh + offset;
	}

	for(i=0;i<r->num_of_sh;i++){
		if(! load_sequence_header(r->sh+i, in)){
			fclose(in);
			free(r->gop);
			free(r->sh);
			free(r);
			return NULL;
		}
	}

	fclose(in);
	
	return r;
}

static void release_gop_entries(GOP_ENTRY *last)
{
	GOP_ENTRY *p;
	GOP_ENTRY *c;

	c = last;

	while(c != NULL){
		p = c->prev;
		free(c);
		c = p;
	}

}
	
static void release_sequence_entries(SEQUENCE_ENTRY *last)
{
	SEQUENCE_ENTRY *p;
	SEQUENCE_ENTRY *c;

	c = last;

	while(c != NULL){
		p = c->prev;
		free(c);
		c = p;
	}
}

static int save_sequence_header(SEQUENCE_HEADER *sh, FILE *fp)
{
	int i;
	
	if(! write_le_int32(sh->orig_h_size, fp) ){
		return 0;
	}
	if(! write_le_int32(sh->orig_v_size, fp) ){
		return 0;
	}
	if(! write_le_int32(sh->h_size, fp) ){
		return 0;
	}
	if(! write_le_int32(sh->v_size, fp) ){
		return 0;
	}
	if(! write_le_int32(sh->aspect_ratio, fp) ){
		return 0;
	}
	if(! write_le_int32(sh->picture_rate, fp) ){
		return 0;
	}
	if(! write_le_int32(sh->bit_rate, fp) ){
		return 0;
	}
	if(! write_le_int32(sh->vbv_buffer_size, fp) ){
		return 0;
	}
	if(! write_le_int32(sh->mpeg1, fp) ){
		return 0;
	}
	
	if(! write_le_int32(sh->has_intra_quantizer_matrix, fp) ){
		return 0;
	}
	if(sh->has_intra_quantizer_matrix){
		for(i=0;i<64;i++){
			fputc(sh->iqm[i], fp);
		}
	}
	
	if(! write_le_int32(sh->has_nonintra_quantizer_matrix, fp) ){
		return 0;
	}
	if(sh->has_nonintra_quantizer_matrix){
		for(i=0;i<64;i++){
			fputc(sh->nqm[i], fp);
		}
	}
	
	if(! write_le_int32(sh->has_sequence_extension, fp) ){
		return 0;
	}
	if(sh->has_sequence_extension){
		if(! write_le_int32(sh->se.profile_and_level, fp)){
			return 0;
		}
		if(! write_le_int32(sh->se.progressive, fp)){
			return 0;
		}
		if(! write_le_int32(sh->se.chroma_format, fp)){
			return 0;
		}
		if(! write_le_int32(sh->se.low_delay, fp)){
			return 0;
		}
		if(! write_le_int32(sh->se.frame_rate_ext_n, fp)){
			return 0;
		}
		if(! write_le_int32(sh->se.frame_rate_ext_d, fp)){
			return 0;
		}
	}
	
	if(! write_le_int32(sh->has_sequence_display_extension, fp)){
		return 0;
	}
	if(sh->has_sequence_display_extension){
		if(! write_le_int32(sh->sd.video_format, fp)){
			return 0;
		}
		if(! write_le_int32(sh->sd.has_color_description, fp)){
			return 0;
		}
		if(! write_le_int32(sh->sd.color_primaries, fp)){
			return 0;
		}
		if(! write_le_int32(sh->sd.transfer_characteristics, fp)){
			return 0;
		}
		if(! write_le_int32(sh->sd.matrix_coefficients, fp)){
			return 0;
		}
		if(! write_le_int32(sh->sd.display_h_size, fp)){
			return 0;
		}
		if(! write_le_int32(sh->sd.display_v_size, fp)){
			return 0;
		}
	}
	
	if(! write_le_int32(sh->has_sequence_scalable_extension, fp)){
		return 0;
	}
	if(sh->has_sequence_scalable_extension){
		if(! write_le_int32(sh->ss.scalable_mode, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.layer, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.lower_layer_prediction_h_size, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.lower_layer_prediction_v_size, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.h_subsampling_facter_m, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.h_subsampling_facter_n, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.v_subsampling_facter_m, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.v_subsampling_facter_n, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.picture_mux_enable, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.mux_to_progressive_sequence, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.pixture_mux_order, fp)){
			return 0;
		}
		if(! write_le_int32(sh->ss.pixture_mux_facter, fp)){
			return 0;
		}
	}

	return 1;
}

static int load_sequence_header(SEQUENCE_HEADER *sh, FILE *fp)
{
	int i;
	
	if(! read_le_int32(fp, &(sh->orig_h_size)) ){
		return 0;
	}
	if(! read_le_int32(fp, &(sh->orig_v_size)) ){
		return 0;
	}
	if(! read_le_int32(fp, &(sh->h_size)) ){
		return 0;
	}
	if(! read_le_int32(fp, &(sh->v_size)) ){
		return 0;
	}
	if(! read_le_int32(fp, &(sh->aspect_ratio)) ){
		return 0;
	}
	if(! read_le_int32(fp, &(sh->picture_rate)) ){
		return 0;
	}
	if(! read_le_int32(fp, &(sh->bit_rate)) ){
		return 0;
	}
	if(! read_le_int32(fp, &(sh->vbv_buffer_size)) ){
		return 0;
	}
	if(! read_le_int32(fp, &(sh->mpeg1)) ){
		return 0;
	}
	
	if(! read_le_int32(fp, &(sh->has_intra_quantizer_matrix)) ){
		return 0;
	}
	if(sh->has_intra_quantizer_matrix){
		for(i=0;i<64;i++){
			sh->iqm[i] = fgetc(fp);
		}
	}
	
	if(! read_le_int32(fp, &(sh->has_nonintra_quantizer_matrix)) ){
		return 0;
	}
	if(sh->has_nonintra_quantizer_matrix){
		for(i=0;i<64;i++){
			sh->nqm[i] = fgetc(fp);
		}
	}
	
	if(! read_le_int32(fp, &(sh->has_sequence_extension)) ){
		return 0;
	}
	if(sh->has_sequence_extension){
		if(! read_le_int32(fp, &(sh->se.profile_and_level))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->se.progressive))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->se.chroma_format))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->se.low_delay))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->se.frame_rate_ext_n))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->se.frame_rate_ext_d))){
			return 0;
		}
	}
	
	if(! read_le_int32(fp, &(sh->has_sequence_display_extension))){
		return 0;
	}
	if(sh->has_sequence_display_extension){
		if(! read_le_int32(fp, &(sh->sd.video_format))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->sd.has_color_description))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->sd.color_primaries))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->sd.transfer_characteristics))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->sd.matrix_coefficients))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->sd.display_h_size))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->sd.display_v_size))){
			return 0;
		}
	}
	
	if(! read_le_int32(fp, &(sh->has_sequence_scalable_extension))){
		return 0;
	}
	if(sh->has_sequence_scalable_extension){
		if(! read_le_int32(fp, &(sh->ss.scalable_mode))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.layer))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.lower_layer_prediction_h_size))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.lower_layer_prediction_v_size))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.h_subsampling_facter_m))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.h_subsampling_facter_n))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.v_subsampling_facter_m))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.v_subsampling_facter_n))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.picture_mux_enable))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.mux_to_progressive_sequence))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.pixture_mux_order))){
			return 0;
		}
		if(! read_le_int32(fp, &(sh->ss.pixture_mux_facter))){
			return 0;
		}
	}

	return 1;
}

