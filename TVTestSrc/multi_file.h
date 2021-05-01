#ifndef MULTI_FILE_H
#define MULTI_FILE_H

typedef struct {
	
	void *private_data;

	int     (* close)(void *multi_file);
	__int64 (* tell)(void *multi_file);
	__int64 (* seek)(void *multi_file, __int64 offset, int origin);
	int     (* read)(void *multi_file, void *buf, int length);
	
	int     (* count)(void *multi_file);
	__int64 (* border)(void *multi_file, int index);
	
} MULTI_FILE;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MULTI_FILE_C
extern MULTI_FILE *open_multi_file(const char *path);
#endif

#ifdef __cplusplus
}
#endif

#endif
