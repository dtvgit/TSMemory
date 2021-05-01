/*******************************************************************
                YUV frame treating interface
 *******************************************************************/

#ifndef FRAME_H
#define FRAME_H

#define UCHAR_CLIP_TABLE_OFFSET 384

typedef struct {
	int width;
	int height;
	
	int in_step;
	int out_step;
	
	int c_offset;
} CONVERSION_PARAMETER;

typedef struct {

	CONVERSION_PARAMETER prm;
	
	int yo; /* Y offset                    */
	
	int yg; /* Y gain                      */

	int bu; /* U(B-Y) to Bule coefficient  */

	int gu; /* U(B-Y) to Green coefficient */
	int gv; /* V(R-Y) to Green coefficient */

	int rv; /* V(R-Y) to Red coefficient   */

} BGR_CONVERSION_PARAMETER;

typedef struct {
	int yu;
	int yv;
	int uu;
	int uv;
	int vu;
	int vv;
} YUY2_CONVERSION_PARAMETER;

typedef struct {
	
	int height;
	int width;

	unsigned char *y;
	unsigned char *u;
	unsigned char *v;
	
} FRAME;

typedef void (__stdcall * YUV_to_BGR)(FRAME *, FRAME *, unsigned char *, BGR_CONVERSION_PARAMETER *);
typedef void (__stdcall * YUV_to_YUY2)(FRAME *, FRAME *, unsigned char *, CONVERSION_PARAMETER *);
typedef void (__stdcall * YUY2_CONVERT)(unsigned char *,int,int,YUY2_CONVERSION_PARAMETER *);
typedef void (* UPSAMPLE_CHROMA)(FRAME *);

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char uchar_clip_table[1024];

extern void copy_frame(FRAME *in, FRAME *out);
extern void copy_2nd_field(FRAME *frame, int picture_structure);

extern void __stdcall yuv444_to_bgr(FRAME *top, FRAME *bottom, unsigned char *out, BGR_CONVERSION_PARAMETER *prm);
extern void __stdcall yuv422_to_bgr(FRAME *top, FRAME *bottom, unsigned char *out, BGR_CONVERSION_PARAMETER *prm);
extern void __stdcall yuv444_to_yuy2(FRAME *top, FRAME *bottom, unsigned char *out, CONVERSION_PARAMETER *prm);
extern void __stdcall yuv422_to_yuy2(FRAME *top, FRAME *bottom, unsigned char *out, CONVERSION_PARAMETER *prm);

extern void __stdcall yuy2_convert(unsigned char *yuy2, int step, int height, YUY2_CONVERSION_PARAMETER *prm);
extern void __stdcall yuy2_convert_none(unsigned char *yuy2, int step, int height, YUY2_CONVERSION_PARAMETER *prm);

extern void __stdcall yuv422_to_bgr_mmx(FRAME *top, FRAME *bottom, unsigned char *out, BGR_CONVERSION_PARAMETER *prm);
extern void __stdcall yuv422_to_yuy2_mmx(FRAME *top, FRAME *bottom, unsigned char *out, CONVERSION_PARAMETER *prm);

extern void __stdcall yuy2_convert_mmx(unsigned char *yuy2, int step, int height, YUY2_CONVERSION_PARAMETER *prm);

extern void __stdcall yuv422_to_bgr_sse2(FRAME *top, FRAME *bottom, unsigned char *out, BGR_CONVERSION_PARAMETER *prm);
extern void __stdcall yuv422_to_yuy2_sse2(FRAME *top, FRAME *bottom, unsigned char *out, CONVERSION_PARAMETER *prm);

extern void __stdcall yuy2_convert_sse2(unsigned char *yuy2, int step, int height, YUY2_CONVERSION_PARAMETER *prm);

extern void upsample_chroma_none(FRAME *p);

extern void upsample_chroma_420i(FRAME *p);
extern void upsample_chroma_420p(FRAME *p);

extern void upsample_chroma_420i_mmx(FRAME *p);
extern void upsample_chroma_420p_mmx(FRAME *p);

extern void upsample_chroma_420i_sse2(FRAME *p);
extern void upsample_chroma_420p_sse2(FRAME *p);

#ifdef __cplusplus
}
#endif

#endif
