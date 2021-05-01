/*******************************************************************
           Killer Tomate - MPEG-2 VIDEO VFAPI Plug-In
 *******************************************************************/
#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "vfapi.h"
#include "vfapi_edit_ext.h"
#include "mpeg2edit.h"
#include "mpeg_video.h"
#include "mpeg_audio.h"

typedef struct {
	MPEG_VIDEO *video;
	MPEG_AUDIO *audio;
} MPEG_DECODER;

HRESULT __stdcall vfGetPluginInfo(LPVF_PluginInfo info);
HRESULT __stdcall vfGetPluginFunc(LPVF_PluginFunc func);
HRESULT __stdcall vfGetPluginFuncEditExt(LPVF_PluginFuncEditExt func_edit_ext);

HRESULT __stdcall open_file(char *path, LPVF_FileHandle out);
HRESULT __stdcall close_file(VF_FileHandle p);
HRESULT __stdcall get_file_info(VF_FileHandle in, LPVF_FileInfo out);
HRESULT __stdcall get_stream_info(VF_FileHandle in, DWORD s, void *out);
static HRESULT get_stream_info_video(MPEG_VIDEO *in, VF_StreamInfo_Video *out);
static HRESULT get_stream_info_audio(MPEG_AUDIO *in, VF_StreamInfo_Audio *out);

HRESULT __stdcall read_data(VF_FileHandle in, DWORD s, void *out);
static HRESULT read_data_video(MPEG_VIDEO *in, VF_ReadData_Video *out);
static HRESULT read_data_audio(MPEG_AUDIO *in, VF_ReadData_Audio *out);

HRESULT __stdcall get_frame_type(VF_FileHandle in, LPVF_FrameType out);
HRESULT __stdcall cut(VF_FileHandle in, LPVF_CutOption opt);

HRESULT __stdcall vfGetPluginInfo(LPVF_PluginInfo info)
{
	if(info == NULL){
		return VF_ERROR;
	}

	if(info->dwSize != sizeof(VF_PluginInfo)){
		return VF_ERROR;
	}

	info->dwAPIVersion = 1;
	info->dwVersion = 1;
	info->dwSupportStreamType = VF_STREAM_VIDEO|VF_STREAM_AUDIO;

	strcpy(info->cPluginInfo, "TVTest Video Plug-In");
	strcpy(info->cFileType, "TVTest Video File |*.tvtp");

	return VF_OK;
}

HRESULT __stdcall vfGetPluginFunc(LPVF_PluginFunc func)
{
	if(func == NULL){
		return VF_ERROR;
	}

	if(func->dwSize != sizeof(VF_PluginFunc)){
		return VF_ERROR;
	}

	func->OpenFile = open_file;
	func->CloseFile = close_file;
	func->GetFileInfo = get_file_info;
	func->GetStreamInfo = get_stream_info;
	func->ReadData = read_data;

	return VF_OK;
}

HRESULT __stdcall vfGetPluginFuncEditExt(LPVF_PluginFuncEditExt func_edit_ext)
{
	if(func_edit_ext == NULL){
		return VF_ERROR;
	}

	if(func_edit_ext->dwSize != sizeof(VF_PluginFuncEditExt)){
		return VF_ERROR;
	}

	func_edit_ext->GetFrameType = get_frame_type;
	func_edit_ext->Cut = cut;
	
	return VF_OK;
}

HRESULT __stdcall open_file(char *path, LPVF_FileHandle out)
{
	MPEG_DECODER **w;

	w = (MPEG_DECODER **)out;

	*w = (MPEG_DECODER *)malloc(sizeof(MPEG_DECODER));
	if(*w == NULL){
		return VF_ERROR;
	}

	(*w)->video = open_mpeg_video(path);
	(*w)->audio = open_mpeg_audio(path);

	if( ((*w)->video == NULL) && ((*w)->audio == NULL) ){
		free(*w);
		*w = NULL;
		return VF_ERROR;
	}

	return VF_OK;
}

HRESULT __stdcall close_file(VF_FileHandle p)
{
	MPEG_DECODER *w;

	w = (MPEG_DECODER *)p;

	if(w->video){
		close_mpeg_video(w->video);
		w->video = NULL;
	}
	if(w->audio){
		close_mpeg_audio(w->audio);
		w->audio = NULL;
	}

	free(w);
	
	return VF_OK;
}

HRESULT __stdcall get_file_info(VF_FileHandle in, LPVF_FileInfo out)
{
	MPEG_DECODER *w;

	w = (MPEG_DECODER *)in;
	
	if(out == NULL){
		return VF_ERROR;
	}

	if(out->dwSize != sizeof(VF_FileInfo)){
		return VF_ERROR;
	}
	out->dwHasStreams = 0;
	if(w->video){
		out->dwHasStreams |= VF_STREAM_VIDEO;
	}
	if(w->audio){
		out->dwHasStreams |= VF_STREAM_AUDIO;
	}

	return VF_OK;
}

HRESULT __stdcall get_stream_info(VF_FileHandle in, DWORD s, void *out)
{
	MPEG_DECODER *w;

	w = (MPEG_DECODER *)in;
	
	if(s == VF_STREAM_VIDEO){
		return get_stream_info_video(w->video, (VF_StreamInfo_Video *)out);
	}else if(s == VF_STREAM_AUDIO){
		return get_stream_info_audio(w->audio, (VF_StreamInfo_Audio *)out);
	}
	return VF_ERROR;
}

static HRESULT get_stream_info_video(MPEG_VIDEO *in, VF_StreamInfo_Video *out)
{
	if(out == NULL){
		return VF_ERROR;
	}
	
	if(out->dwSize != sizeof(VF_StreamInfo_Video)){
		return VF_ERROR;
	}

	out->dwLengthL = (DWORD)(in->total & 0xffffffff);
	out->dwLengthH = (DWORD)(in->total >> 32);
	out->dwRate = in->rate;
	out->dwScale = in->scale;
	out->dwWidth = in->width;
	out->dwHeight = in->height;
	out->dwBitCount = 24;

	return VF_OK;
}

static HRESULT get_stream_info_audio(MPEG_AUDIO *in, VF_StreamInfo_Audio *out)
{
	if(out == NULL){
		return VF_ERROR;
	}

	if(out->dwSize != sizeof(VF_StreamInfo_Audio)){
		return VF_ERROR;
	}

	out->dwLengthL = (DWORD)(in->sample & 0xffffffff);
	out->dwLengthH = (DWORD)(in->sample >> 32);
	out->dwRate = in->frequency;
	out->dwScale = 1;
	out->dwChannels = in->channel;
	out->dwBitsPerSample = 16;
	out->dwBlockAlign = in->channel * sizeof(short);

	return VF_OK;
}
	
HRESULT __stdcall read_data(VF_FileHandle in, DWORD s, void *out)
{
	MPEG_DECODER *w;

	w = (MPEG_DECODER *)in;
	
	if(s == VF_STREAM_VIDEO){
		return read_data_video(w->video, (VF_ReadData_Video *)out);
	}else if(s == VF_STREAM_AUDIO){
		return read_data_audio(w->audio, (VF_ReadData_Audio *)out);
	}

	return VF_ERROR;
}

static HRESULT read_data_video(MPEG_VIDEO *in, VF_ReadData_Video *out)
{
	int height,save_height;
	OUT_BUFFER_ELEMENT *top;
	OUT_BUFFER_ELEMENT *bottom;
	
	unsigned char *dst;

	__int64 frame;
	
	if(out == NULL){
		return VF_ERROR;
	}
	
	if(out->dwSize != sizeof(VF_ReadData_Video)){
		return VF_ERROR;
	}

	frame = out->dwFrameNumberL;
	frame += ((__int64)out->dwFrameNumberH) << 32;

	EnterCriticalSection(&(in->lock));

	switch(in->config.field_mode){
	case 0: /* keep original frame */
		top = read_frame(in, frame);
		bottom = top;
		if(top == NULL){
			__asm{emms};
			LeaveCriticalSection(&(in->lock));
			return VF_ERROR;
		}
		break;
	case 1: /* top field first */
		top = read_frame(in, frame);
		if(top == NULL){
			__asm{emms};
			LeaveCriticalSection(&(in->lock));
			return VF_ERROR;
		}
		bottom = NULL;
		if( (!top->prm.top_field_first) && (!top->prm.repeat_first_field) ){
			bottom = read_frame(in, frame+1);
		}
		if(bottom == NULL){
			bottom = top;
		}

		break;
	case 2: /* bottom field first */
		bottom = read_frame(in, frame);
		if(bottom == NULL){
			__asm{emms};
			LeaveCriticalSection(&(in->lock));
			return VF_ERROR;
		}
		top = NULL;
		if( bottom->prm.top_field_first && (!bottom->prm.repeat_first_field) ){
			top = read_frame(in, frame+1);
		}
		if(top == NULL){
			top = bottom;
		}
		break;
	default:
		top = read_frame(in, frame);
		bottom = top;
		if(top == NULL){
			__asm{emms};
			LeaveCriticalSection(&(in->lock));
			return VF_ERROR;
		}
	}
	
	dst = (unsigned char *)out->lpData;
	
	in->bgr_prm.prm.out_step = out->lPitch;
	
	save_height = in->bgr_prm.prm.height;
	height = save_height;
	if (height > top->data.height) {
		height = top->data.height;
	}
	if (height > bottom->data.height) {
		height = bottom->data.height;
	}
	in->bgr_prm.prm.height = height;
	
	in->to_bgr(&(top->data), &(bottom->data), dst, &(in->bgr_prm));
	
	in->bgr_prm.prm.height = save_height;

	__asm{emms};
	LeaveCriticalSection(&(in->lock));

	return VF_OK;
}

static HRESULT read_data_audio(MPEG_AUDIO *in, VF_ReadData_Audio *out)
{
	__int64 sample;
	
	if(in == NULL){
		return VF_ERROR;
	}
	if(out == NULL){
		return VF_ERROR;
	}
	if(out->dwSize != sizeof(VF_ReadData_Audio)){
		return VF_ERROR;
	}

	sample = out->dwSamplePosL;
	sample |= ((__int64)out->dwSamplePosH) << 32;

	out->dwReadedSampleCount = read_mpeg_audio(in, sample, out->lpBuf, out->dwSampleCount);
	if(out->dwReadedSampleCount){
		return VF_OK;
	}

	return VF_ERROR;
}

HRESULT __stdcall get_frame_type(VF_FileHandle in, LPVF_FrameType out)
{
	__int64 frame;

	MPEG_VIDEO *w;
	int type;

	if(out == NULL){
		return VF_ERROR;
	}

	if(out->dwSize != sizeof(VF_FrameType)){
		return VF_ERROR;
	}

	w = ((MPEG_DECODER *)in)->video;
	if(w == NULL){
		return VF_ERROR;
	}

	frame = out->dwFrameNumberL;
	frame += ((__int64)(out->dwFrameNumberH)) << 32;

	EnterCriticalSection(&(w->lock));
	type = get_picture_coding_type(w, frame);
	LeaveCriticalSection(&(w->lock));

	switch(type){
	case 1:
		/* I picture */
		out->dwFrameType = VF_FRAME_TYPE_STARTABLE|VF_FRAME_TYPE_ENDABLE;
		break;
	case 2:
		/* P picture */
		out->dwFrameType = VF_FRAME_TYPE_ENDABLE;
		break;
	case 3:
		/* B picture */
		out->dwFrameType = VF_FRAME_TYPE_UNEDITABLE;
		break;
	case 4:
		/* B picture with closed gop */
		out->dwFrameType = VF_FRAME_TYPE_STARTABLE;
		break;
	default:
		/* unknown picture */
		out->dwFrameType = VF_FRAME_TYPE_UNEDITABLE;
	}

	return VF_OK;
}

HRESULT __stdcall cut(VF_FileHandle in, LPVF_CutOption opt)
{
	__int64 in_frame;
	__int64 out_frame;
	
	MPEG_VIDEO *w;
	MPEG2EDIT mpeg2edit;

	if(opt == NULL){
		return VF_ERROR;
	}

	if(opt->dwSize != sizeof(VF_CutOption)){
		return VF_ERROR;
	}

	w = ((MPEG_DECODER *)in)->video;
	if(w == NULL){
		return VF_ERROR;
	}

	if( !open_mpeg2edit(w->bitstream.path, &mpeg2edit) ){
		return VF_ERROR;
	}

	if(opt->Callback != NULL){
		mpeg2edit.callback = opt->Callback;
	}

	in_frame = opt->dwInPointH;
	in_frame <<= 32;
	in_frame += opt->dwInPointL;

	out_frame = opt->dwOutPointH;
	out_frame <<= 32;
	out_frame += opt->dwOutPointL;

	mpeg2edit.edit(&mpeg2edit, opt->lpOutputFileName, in_frame, out_frame);

	mpeg2edit.close(&mpeg2edit);

	return VF_OK;
}
