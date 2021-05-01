/*******************************************************************
                         Registry Interface
 *******************************************************************/
#ifndef REGISTRY_H
#define REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef REGISTRY_C
extern int get_color_conversion_type();
extern int get_idct_type();
extern int get_simd_mode();
extern int get_field_mode();
extern int get_resize_mode();
extern int get_filecheck_limit();
extern int get_color_matrix();
extern int get_gl_mode();
extern int get_file_mode();
extern int get_yuy2_mode();
#endif /* REGISTRY_C */

#ifdef __cplusplus
}
#endif
	
#endif /* REGISTRY_H */
