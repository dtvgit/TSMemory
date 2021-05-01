#include <memory>
#include "mpeg_video.h"
#include "mpeg_audio.h"
#include "input.h"

//////////////////////////////////////////////////////////////////////////////

class M2V {
	MPEG_VIDEO *video;

	void ReadFields(int frame, OUT_BUFFER_ELEMENT *& top, OUT_BUFFER_ELEMENT *& bottom) const {
		top = bottom = read_frame(video, frame);

		if(top == NULL)
			return;

		switch(video->config.field_mode) {
		case 0: // keep original frame
			break;

		case 1: // top field first
			if(!top->prm.top_field_first && !top->prm.repeat_first_field)
				bottom = read_frame(video, frame + 1);

			if(bottom == NULL)
				bottom = top;

			break;

		case 2: // bottom field first
			if(bottom->prm.top_field_first && !bottom->prm.repeat_first_field)
				top = read_frame(video, frame + 1);

			if(top == NULL)
				top = bottom;

			break;
		}
	}

public:
	M2V(const char *file) {
		video = open_mpeg_video((char *)file);
	}

	~M2V() {
		if(video != NULL) {
			close_mpeg_video(video);
			video = NULL;
		}
	}

	MPEG_VIDEO *operator->() const {
		return video;
	}

	operator MPEG_VIDEO *() const {
		return video;
	}

	bool ReadYUV422(int frame, void *buf, int pitch) const {

		int i,save_height;
		OUT_BUFFER_ELEMENT *fields[2];

		EnterCriticalSection(&(video->lock));

		ReadFields(frame, fields[0], fields[1]);

		if(fields[0] == NULL) {
			__asm {emms};
			LeaveCriticalSection(&(video->lock));
			return false;
		}

		video->bgr_prm.prm.out_step = pitch;
		
		save_height = video->bgr_prm.prm.height;
		for(i=0;i<2;i++){
			if (video->bgr_prm.prm.height > fields[i]->data.height) {
				video->bgr_prm.prm.height = fields[i]->data.height;
			}
		}
		
		video->to_yuy2(&(fields[0]->data), &(fields[1]->data), (unsigned char *)buf, &(video->bgr_prm.prm));
		video->yuy2_cc((unsigned char *)buf, pitch, video->bgr_prm.prm.height, &(video->ycc_prm));
		
		video->bgr_prm.prm.height = save_height;
		
		__asm {emms};
		LeaveCriticalSection(&(video->lock));
		
		return true;
	}
};

//////////////////////////////////////////////////////////////////////////////

class M2A {
	MPEG_AUDIO *audio;

public:
	M2A(const char *file) {
		audio = open_mpeg_audio((char *)file);
	}

	~M2A() {
		if(audio != NULL)
			close_mpeg_audio(audio);
	}

	MPEG_AUDIO *operator->() const {
		return audio;
	}

	operator MPEG_AUDIO *() const {
		return audio;
	}

	int GetChannels() const {
		return 2;
	}

	int GetBitsPerSample() const {
		return 16;
	}
};

//////////////////////////////////////////////////////////////////////////////

class ForAviutl {
	M2V m2v;
	M2A m2a;
	BITMAPINFOHEADER bih;
	WAVEFORMATEX wfex;
	int pitch;

	ForAviutl(LPSTR file) : m2v(file), m2a(file) {
		memset(&bih, 0, sizeof(bih));
		memset(&wfex, 0, sizeof(wfex));

		if(m2v != NULL) {
			bih.biSize = sizeof(bih);
			bih.biWidth = m2v->width;
			bih.biHeight = m2v->height;
			bih.biPlanes = 1;
			bih.biBitCount = 16;
			bih.biCompression = '2YUY';

			pitch = ((bih.biBitCount * bih.biWidth + 31) & ~31) >> 3;
			bih.biSizeImage = pitch * bih.biHeight;
		}

		if(m2a != NULL) {
			wfex.wFormatTag = WAVE_FORMAT_PCM;
			wfex.nChannels = m2a->channel;
			wfex.wBitsPerSample = m2a.GetBitsPerSample();
			wfex.nBlockAlign = wfex.nChannels * wfex.wBitsPerSample / 8;
			wfex.nSamplesPerSec = m2a->frequency;
			wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
		}
	}

	bool GetInfo(INPUT_INFO *iip) {
		iip->flag = 0;

		if(m2v != NULL) {
			iip->flag |= INPUT_INFO_FLAG_VIDEO;
			iip->rate = m2v->rate;
			iip->scale = m2v->scale;
			iip->n = (int)m2v->total;
			iip->format = &bih;
			iip->format_size = sizeof(bih);
			iip->handler = 0;
		}

		if(m2a != NULL) {
			iip->flag |= INPUT_INFO_FLAG_AUDIO;
			iip->audio_n = (int)m2a->sample;
			iip->audio_format = &wfex;
			iip->audio_format_size = sizeof(wfex);
		}

		return true;
	}

	int ReadVideo(int frame, void *buf) {
		int bytes = 0;

		if(m2v != NULL && m2v.ReadYUV422(frame, buf, pitch))
			bytes = bih.biSizeImage;

		return bytes;
	}

	int ReadAudio(int start, int length, void *buf) {
		int samples = 0;

		if(m2a != NULL)
			samples = read_mpeg_audio(m2a, start, buf, length);

		return samples;
	}

	static INPUT_HANDLE __cdecl func_open(LPSTR file) {
		std::auto_ptr<ForAviutl> handle(new ForAviutl(file));
		return (handle->m2v != NULL || handle->m2a != NULL) ? handle.release() : NULL;
	}

	static BOOL __cdecl func_close(INPUT_HANDLE ih) {
		delete (ForAviutl *)ih;
		return TRUE;
	}

	static BOOL __cdecl func_info_get(INPUT_HANDLE ih, INPUT_INFO *iip) {
		return ((ForAviutl *)ih)->GetInfo(iip);
	}

	static int __cdecl func_read_video(INPUT_HANDLE ih, int frame, void *buf) {
		return ((ForAviutl *)ih)->ReadVideo(frame, buf);
	}

	static int __cdecl func_read_audio(INPUT_HANDLE ih, int start, int length, void *buf) {
		return ((ForAviutl *)ih)->ReadAudio(start, length, buf);
	}

public:
	static INPUT_PLUGIN_TABLE *GetInputPluginTable() {
		#define _FILE_TYPE_ "*.tvtv"
		static INPUT_PLUGIN_TABLE table = {
			INPUT_PLUGIN_FLAG_VIDEO | INPUT_PLUGIN_FLAG_AUDIO,
			"TVTest Video Reader",
			"TVTest Video File (" _FILE_TYPE_ ")\0" _FILE_TYPE_ "\0",
			"TVTest Video Reader ver.0.1.0.7.14",
			NULL,
			NULL,
			(INPUT_HANDLE (*)(LPSTR))func_open,
			(BOOL (*)(INPUT_HANDLE))func_close,
			(BOOL (*)(INPUT_HANDLE, INPUT_INFO *))func_info_get,
			(int (*)(INPUT_HANDLE, int, void *))func_read_video,
			(int (*)(INPUT_HANDLE, int, int, void *))func_read_audio,
			NULL,
			NULL,
		};
		#undef _FILE_TYPE_
		return &table;
	}
	static HINSTANCE m_hinst;
};

HINSTANCE ForAviutl::m_hinst=NULL;

//////////////////////////////////////////////////////////////////////////////

extern "C" __declspec(dllexport)
INPUT_PLUGIN_TABLE * __stdcall GetInputPluginTable() {
	return ForAviutl::GetInputPluginTable();
}

//////////////////////////////////////////////////////////////////////////////

/*
BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpvReserved)
{
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		ForAviutl::m_hinst=hinstDLL;
		break;
	}
	return TRUE;
}
*/

extern "C" HINSTANCE get_dll_handle()
{
	return ForAviutl::m_hinst;
}
