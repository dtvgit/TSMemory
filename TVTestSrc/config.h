/*******************************************************************
                    m2v.vfp config structure
 *******************************************************************/
#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
	int bt601;
	int yuy2;
	int aspect_ratio;
	int field_mode;
	int simd;
	int idct_type;
	int color_matrix;
	int gl;
} M2V_CONFIG;

#define M2V_CONFIG_YUV_FULL_RANGE        0
#define M2V_CONFIG_YUV_BT601_RANGE       1

#define M2V_CONFIG_YUY2_CONVERT_NONE     0
#define M2V_CONFIG_YUY2_CONVERT_BT601    1
#define M2V_CONFIG_YUY2_CONVERT_BT709    2
#define M2V_CONFIG_YUY2_CONVERT_FCC      3
#define M2V_CONFIG_YUY2_CONVERT_SMPTE240 4

#define M2V_CONFIG_IGNORE_ASPECT_RATIO   0
#define M2V_CONFIG_USE_ASPECT_RATIO      1

#define M2V_CONFIG_FRAME_KEEP_ORIGINAL   0
#define M2V_CONFIG_FRAME_TOP_FIRST       1
#define M2V_CONFIG_FRAME_BOTTOM_FIRST    2

#define M2V_CONFIG_USE_MMX               0x001
#define M2V_CONFIG_USE_SSE               0x002
#define M2V_CONFIG_USE_SSE2              0x004
#define M2V_CONFIG_USE_3DNOW             0x100
#define M2V_CONFIG_USE_E3DNOW            0x200

#define M2V_CONFIG_IDCT_REFERENCE        0
#define M2V_CONFIG_IDCT_LLM_INT          1
#define M2V_CONFIG_IDCT_AP922_INT        2

#define M2V_CONFIG_GL_NEVER_SAVE         0x01
#define M2V_CONFIG_GL_ALWAYS_SCAN        0x02

#define M2V_CONFIG_SINGLE_FILE           0
#define M2V_CONFIG_MULTI_FILE            1

#endif /* CONFIG_H */
