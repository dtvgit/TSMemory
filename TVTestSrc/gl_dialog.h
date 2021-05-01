#ifndef GL_DIALOG_H
#define GL_DIALOG_H

typedef struct {
	void *private_data;
	int  (*update)(void *gl_dialog, int percent);
	int  (*is_cancel)(void *gl_dialog);
	void (*delete)(void *gl_dialog);
} GL_DIALOG;

#ifdef __cplusplus
extern "C" {
#endif

extern GL_DIALOG *create_gl_dialog(int mode);

#define GL_DIALOG_MODE_GUI 0
#define GL_DIALOG_MODE_CUI 1

#ifdef __cplusplus
}
#endif

#endif
