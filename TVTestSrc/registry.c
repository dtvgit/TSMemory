/*******************************************************************
 Registry Module
 *******************************************************************/
#include <windows.h>
#include <winreg.h>

#include "config.h"
#include "plugin.h"
#include "filename.h"

#define REGISTRY_C
#include "registry.h"

int get_color_conversion_type();
int get_idct_type();
int get_simd_mode();
int get_field_mode();
int get_resize_mode();
int get_file_check_limit();
int get_color_matrix();
int get_gl_mode();
int get_file_mode();
int get_yuy2_mode();

//static const char REGISTRY_POSITION[] = "Software\\marumo\\mpeg2vid_vfp";

static void get_ini_filename(char *filename)
{
	GetModuleFileNameA((HMODULE)get_dll_handle(),filename,MAX_PATH);
	strcpy(read_suffix(filename),".ini");
}

int get_color_conversion_type()
{
	char inifile[MAX_PATH];
	int value;

	get_ini_filename(inifile);
	value=GetPrivateProfileIntA("settings","re_map",1,inifile);
	if(value){
		return 1;
	}

	return 0;
}

int get_idct_type()
{
	char inifile[MAX_PATH];
	int value;

	get_ini_filename(inifile);
	value=GetPrivateProfileIntA("settings","idct_func",2,inifile);

	return value;
}

int get_simd_mode()
{
	char inifile[MAX_PATH];
	int value;

	get_ini_filename(inifile);
	value=GetPrivateProfileIntA("settings","simd",0,inifile);

	return value;
}

int get_field_mode()
{
	char inifile[MAX_PATH];
	int value;

	get_ini_filename(inifile);
	value=GetPrivateProfileIntA("settings","field_order",0,inifile);

	return value;
}

int get_resize_mode()
{
	char inifile[MAX_PATH];
	int value;

	get_ini_filename(inifile);
	value=GetPrivateProfileIntA("settings","aspect_ratio",M2V_CONFIG_USE_ASPECT_RATIO,inifile);

	return value;
}

int get_filecheck_limit()
{
	char inifile[MAX_PATH];
	int value;

	get_ini_filename(inifile);
	value=GetPrivateProfileIntA("settings","limit",1024*1024*8,inifile);

	return value;
}

int get_color_matrix()
{
	char inifile[MAX_PATH];
	int value;

	get_ini_filename(inifile);
	value=GetPrivateProfileIntA("settings","color_matrix",0,inifile);

	return value;
}

int get_gl_mode()
{
#if 0
	char inifile[MAX_PATH];
	int value;

	get_ini_filename(inifile);
	value=GetPrivateProfileIntA("settings","gl",0,inifile);

	return value;
#else
	return M2V_CONFIG_GL_NEVER_SAVE;
#endif
}

int get_file_mode()
{
#if 0
	char inifile[MAX_PATH];
	int value;

	get_ini_filename(inifile);
	value=GetPrivateProfileIntA("settings","file",M2V_CONFIG_SINGLE_FILE,inifile);

	if(value != M2V_CONFIG_MULTI_FILE){
		return M2V_CONFIG_SINGLE_FILE;
	}

	return value;
#else
	return M2V_CONFIG_SINGLE_FILE;
#endif
}

int get_yuy2_mode()
{
	char inifile[MAX_PATH];
	int value;

	get_ini_filename(inifile);
	value=GetPrivateProfileIntA("settings","yuy2_matrix",M2V_CONFIG_YUY2_CONVERT_NONE,inifile);

	return value;
}

