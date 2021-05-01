plugin.obj: plugin.cpp mpeg_video.h video_stream.h sequence_header.h frame.h picture_header.h out_buffer.h macroblock.h block.h mc.h slice_header.h config.h gop.h timecode.h picture.h idct.h resize.h mpeg_audio.h audio_stream.h pcm_buffer.h layer2.h input.h 
	$(CC) $(CFLAG) plugin.cpp

001.obj: 001.c 001.h 
	$(CC) $(CFLAG) 001.c

audio_stream.obj: audio_stream.c bitstream.h multi_file.h 001.h pes.h stream_type.h memory_stream.h memory_buffer.h layer2.h audio_stream.h 
	$(CC) $(CFLAG) audio_stream.c

bitstream.obj: bitstream.c 001.h bitstream.h multi_file.h 
	$(CC) $(CFLAG) bitstream.c

block.obj: block.c scan.h dct_coefficient.h video_stream.h block.h 
	$(CC) $(CFLAG) block.c

dct_coefficient.obj: dct_coefficient.c dct_coefficient.h video_stream.h 
	$(CC) $(CFLAG) dct_coefficient.c

endian.obj: endian.c endian.h 
	$(CC) $(CFLAG) endian.c

filename.obj: filename.c filename.h 
	$(CC) $(CFLAG) filename.c

frame.obj: frame.c frame.h 
	$(CC) $(CFLAG) frame.c

gl_dialog.obj: gl_dialog.c resource.h gl_dialog.h 
	$(CC) $(CFLAG) gl_dialog.c

gop.obj: gop.c sequence_header.h video_stream.h frame.h picture_header.h out_buffer.h macroblock.h block.h mc.h slice_header.h config.h gop.h timecode.h 
	$(CC) $(CFLAG) gop.c

gop_list.obj: gop_list.c endian.h gl_dialog.h gop_list.h video_stream.h sequence_header.h frame.h picture_header.h out_buffer.h macroblock.h block.h mc.h slice_header.h config.h gop.h timecode.h 
	$(CC) $(CFLAG) gop_list.c

idct_ap922_int.obj: idct_ap922_int.c idct_clip_table.h idct_ap922_int.h 
	$(CC) $(CFLAG) idct_ap922_int.c

idct_clip_table.obj: idct_clip_table.c idct_clip_table.h 
	$(CC) $(CFLAG) idct_clip_table.c

idct_llm_int.obj: idct_llm_int.c idct_clip_table.h idct_llm_int.h 
	$(CC) $(CFLAG) idct_llm_int.c

idct_reference.obj: idct_reference.c idct_clip_table.h idct_reference.h 
	$(CC) $(CFLAG) idct_reference.c

idct_reference_sse.obj: idct_reference_sse.c idct_clip_table.h idct_reference_sse.h 
	$(CC) $(CFLAG) idct_reference_sse.c

instance_manager.obj: instance_manager.c instance_manager.h 
	$(CC) $(CFLAG) instance_manager.c

layer2.obj: layer2.c layer2.h memory_stream.h 
	$(CC) $(CFLAG) layer2.c

m2v.obj: m2v.c vfapi.h vfapi_edit_ext.h mpeg2edit.h mpeg_video.h video_stream.h sequence_header.h frame.h picture_header.h out_buffer.h macroblock.h block.h mc.h slice_header.h config.h gop.h timecode.h picture.h idct.h resize.h mpeg_audio.h audio_stream.h pcm_buffer.h layer2.h 
	$(CC) $(CFLAG) m2v.c

macroblock.obj: macroblock.c macroblock.h video_stream.h block.h mc.h frame.h 
	$(CC) $(CFLAG) macroblock.c

mc.obj: mc.c mc.h frame.h 
	$(CC) $(CFLAG) mc.c

memory_buffer.obj: memory_buffer.c 001.h memory_buffer.h 
	$(CC) $(CFLAG) memory_buffer.c

memory_stream.obj: memory_stream.c memory_stream.h 
	$(CC) $(CFLAG) memory_stream.c

mpeg2edit.obj: mpeg2edit.c bitstream.h multi_file.h vfapi.h mpeg2edit.h 
	$(CC) $(CFLAG) mpeg2edit.c

mpeg_audio.obj: mpeg_audio.c instance_manager.h mpeg_audio.h audio_stream.h pcm_buffer.h layer2.h 
	$(CC) $(CFLAG) mpeg_audio.c

mpeg_video.obj: mpeg_video.c gop_list.h video_stream.h sequence_header.h frame.h picture_header.h out_buffer.h macroblock.h block.h mc.h slice_header.h config.h gop.h timecode.h filename.h registry.h instance_manager.h idct_llm_int.h idct_llm_mmx.h idct_reference.h idct_reference_sse.h idct_ap922_int.h idct_ap922_mmx.h idct_ap922_sse.h idct_ap922_sse2.h mpeg_video.h picture.h idct.h resize.h 
	$(CC) $(CFLAG) mpeg_video.c

multi_file.obj: multi_file.c registry.h config.h multi_file.h 
	$(CC) $(CFLAG) multi_file.c

out_buffer.obj: out_buffer.c out_buffer.h frame.h 
	$(CC) $(CFLAG) out_buffer.c

pcm_buffer.obj: pcm_buffer.c pcm_buffer.h 
	$(CC) $(CFLAG) pcm_buffer.c

pes.obj: pes.c 001.h memory_stream.h pes.h stream_type.h 
	$(CC) $(CFLAG) pes.c

picture.obj: picture.c picture.h block.h video_stream.h mc.h frame.h macroblock.h slice_header.h idct.h 
	$(CC) $(CFLAG) picture.c

picture_header.obj: picture_header.c scan.h picture_header.h video_stream.h out_buffer.h frame.h macroblock.h block.h mc.h 
	$(CC) $(CFLAG) picture_header.c

program_stream.obj: program_stream.c pes.h stream_type.h bitstream.h multi_file.h registry.h program_stream.h 
	$(CC) $(CFLAG) program_stream.c

registry.obj: registry.c config.h registry.h 
	$(CC) $(CFLAG) registry.c

resize.obj: resize.c resize.h frame.h config.h sequence_header.h video_stream.h picture_header.h out_buffer.h macroblock.h block.h mc.h slice_header.h 
	$(CC) $(CFLAG) resize.c

scan.obj: scan.c scan.h 
	$(CC) $(CFLAG) scan.c

sequence_header.obj: sequence_header.c scan.h sequence_header.h video_stream.h frame.h picture_header.h out_buffer.h macroblock.h block.h mc.h slice_header.h config.h 
	$(CC) $(CFLAG) sequence_header.c

slice_header.obj: slice_header.c slice_header.h video_stream.h block.h 
	$(CC) $(CFLAG) slice_header.c

timecode.obj: timecode.c timecode.h video_stream.h 
	$(CC) $(CFLAG) timecode.c

transport_stream.obj: transport_stream.c pes.h stream_type.h 001.h registry.h multi_file.h transport_stream.h 
	$(CC) $(CFLAG) transport_stream.c

video_stream.obj: video_stream.c program_stream.h stream_type.h transport_stream.h 001.h multi_file.h video_stream.h 
	$(CC) $(CFLAG) video_stream.c

block_mmx.obj: block_mmx.asm
	$(ASM) $(AFLAG) block_mmx.asm

frame_mmx.obj: frame_mmx.asm
	$(ASM) $(AFLAG) frame_mmx.asm

frame_sse2.obj: frame_sse2.asm
	$(ASM) $(AFLAG) frame_sse2.asm

idct_ap922_mmx.obj: idct_ap922_mmx.asm
	$(ASM) $(AFLAG) idct_ap922_mmx.asm

idct_ap922_sse.obj: idct_ap922_sse.asm
	$(ASM) $(AFLAG) idct_ap922_sse.asm

idct_ap922_sse2.obj: idct_ap922_sse2.asm
	$(ASM) $(AFLAG) idct_ap922_sse2.asm

idct_llm_mmx.obj: idct_llm_mmx.asm
	$(ASM) $(AFLAG) idct_llm_mmx.asm

mc_mmx.obj: mc_mmx.asm
	$(ASM) $(AFLAG) mc_mmx.asm

mc_sse.obj: mc_sse.asm
	$(ASM) $(AFLAG) mc_sse.asm

mc_sse2.obj: mc_sse2.asm
	$(ASM) $(AFLAG) mc_sse2.asm

OBJ = plugin.obj 001.obj audio_stream.obj bitstream.obj block.obj dct_coefficient.obj endian.obj filename.obj frame.obj gl_dialog.obj gop.obj gop_list.obj idct_ap922_int.obj idct_clip_table.obj idct_llm_int.obj idct_reference.obj idct_reference_sse.obj instance_manager.obj layer2.obj m2v.obj macroblock.obj mc.obj memory_buffer.obj memory_stream.obj mpeg2edit.obj mpeg_audio.obj mpeg_video.obj multi_file.obj out_buffer.obj pcm_buffer.obj pes.obj picture.obj picture_header.obj program_stream.obj registry.obj resize.obj scan.obj sequence_header.obj slice_header.obj timecode.obj transport_stream.obj video_stream.obj block_mmx.obj frame_mmx.obj frame_sse2.obj idct_ap922_mmx.obj idct_ap922_sse.obj idct_ap922_sse2.obj idct_llm_mmx.obj mc_mmx.obj mc_sse.obj mc_sse2.obj 

AOBJ: block_mmx.obj frame_mmx.obj frame_sse2.obj idct_ap922_mmx.obj idct_ap922_sse.obj idct_ap922_sse2.obj idct_llm_mmx.obj mc_mmx.obj mc_sse.obj mc_sse2.obj 

