#include <stdio.h>
#include <stdlib.h>
//#include <io.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

#include <windows.h> /* for CharNext() */
#include "registry.h" /* for get_file_mode() */
#include "config.h"

#define MULTI_FILE_C
#include "multi_file.h"

/* by HDUSTestÇÃíÜÇÃêl */
#include "shared_memory.h"
#define _open(filename,oflag)	open_shared_memory(filename)
#define _close					shm_close
#define _read					shm_read
#define _telli64				shm_tell
#define _lseeki64				shm_seek

/************* type and function definition **************/

typedef struct {
	__int64 length;
	__int64 offset;
	int   fd;
	char *path;
	int   padding[2];
} MF_FILE_INFO;
	

typedef struct {
	__int64  total;
	int count;
	int index;
	MF_FILE_INFO *info;
} MF_PRIVATE_DATA;

MULTI_FILE *open_multi_file(const char *path);

static int mf_close(void *multi_file);
static __int64 mf_tell(void *multi_file);
static __int64 mf_seek(void *multi_file, __int64 offset, int origin);
static int mf_read(void *multi_file, void *buf, int length);
static __int64 sf_tell(void *multi_file);
static __int64 sf_seek(void *multi_file, __int64 offset, int origin);
static int sf_read(void *multi_file, void *buf, int length);

static int mf_count(void *multi_file);
static __int64 mf_border(void *multi_file, int index);

static int try_next(char *path);

/******************** implementation ********************/

MULTI_FILE *open_multi_file(const char *path)
{
	__int64 len;
	
	MULTI_FILE *r;
	MF_PRIVATE_DATA *prv;
	int fd;

	int i,n;
	char *work;
	void *tmp;

	int mode;
	
	r = (MULTI_FILE *)calloc(1, sizeof(MULTI_FILE));
	if(r == NULL){
		return NULL;
	}

	prv = (MF_PRIVATE_DATA *)calloc(1, sizeof(MF_PRIVATE_DATA));
	if(prv == NULL){
		free(r);
		return NULL;
	}

	fd = _open(path, _O_BINARY|_O_RDONLY|_O_SEQUENTIAL);
	if(fd < 0){
		free(prv);
		free(r);
		return NULL;
	}

	n = strlen(path)+1;
	work = (char *)malloc(n);
	if(work == NULL){
		_close(fd);
		free(prv);
		free(r);
		return NULL;
	}
	memcpy(work, path, n);
	
	prv->info = (MF_FILE_INFO *)malloc(sizeof(MF_FILE_INFO));
	if(prv->info == NULL){
		free(work);
		_close(fd);
		free(prv);
		free(r);
		return NULL;
	}

	mode = get_file_mode();

	while(fd >= 0){
		tmp = realloc(prv->info, sizeof(MF_FILE_INFO)*(prv->count+1));
		if(tmp == NULL){
			_close(fd);
			fd = -1;
			break;
		}
		prv->info = (MF_FILE_INFO *)tmp;

		len = _lseeki64(fd, 0, SEEK_END);
		_lseeki64(fd, 0, SEEK_SET);
		_close(fd);
		
		i = prv->count;

		prv->info[i].length = len;
		prv->info[i].offset = prv->total;
		prv->info[i].fd = -1;
		prv->info[i].path = malloc(n);
		if(prv->info[i].path == NULL){
			break;
		}
		memcpy(prv->info[i].path, work, n);
		
		prv->total += len;

		prv->count += 1;

		if(mode & M2V_CONFIG_MULTI_FILE){
			fd = try_next(work);
		}else{
			fd = -1;
		}
	}

	if(prv->count == 0){
		free(prv->info);
		free(work);
		if(fd >= 0){
			_close(fd);
		}
		free(prv);
		free(r);
		return NULL;
	}

	free(work);

	r->private_data = prv;
	r->close = mf_close;
	if(prv->count == 1){
		r->tell = sf_tell;
		r->seek = sf_seek;
		r->read = sf_read;
		prv->info[0].fd = _open(prv->info[0].path, _O_BINARY|_O_RDONLY|_O_SEQUENTIAL);
	}else{
		r->tell = mf_tell;
		r->seek = mf_seek;
		r->read = mf_read;
	}
	r->count = mf_count;
	r->border = mf_border;
	
	return r;
}

static int mf_close(void *multi_file)
{
	MULTI_FILE *p;
	MF_PRIVATE_DATA *prv;
	
	int i;

	p = (MULTI_FILE *)multi_file;
	prv = (MF_PRIVATE_DATA *)p->private_data;
	if(prv == NULL){
		return 0;
	}
	
	for(i=0;i<prv->count;i++){
		if(prv->info[i].fd >= 0){
			_close(prv->info[i].fd);
			prv->info[i].fd = -1;
		}
		if(prv->info[i].path){
			free(prv->info[i].path);
			prv->info[i].path = NULL;
		}
	}

	free(prv->info);
	free(prv);
	memset(p, 0, sizeof(MULTI_FILE));
	free(p);

	return 1;
}

static __int64 mf_tell(void *multi_file)
{
	__int64 r;

	MULTI_FILE *p;
	MF_PRIVATE_DATA *prv;

	int i;
	
	p = (MULTI_FILE *)multi_file;
	prv = (MF_PRIVATE_DATA *)p->private_data;

	if(prv == NULL){
		return 0;
	}

	i = prv->index;
	if(i == prv->count){
		return prv->total;
	}

	r = prv->info[i].offset;
	if(prv->info[i].fd >= 0){
		r += _telli64(prv->info[i].fd);
	}

	return r;
}

static __int64 mf_seek(void *multi_file, __int64 offset, int origin)
{
	int i;
	__int64 r;
	
	MULTI_FILE *p;
	MF_PRIVATE_DATA *prv;

	p = (MULTI_FILE *)multi_file;
	prv = (MF_PRIVATE_DATA *)p->private_data;

	if(prv == NULL){
		return 0;
	}

	i = prv->index;
	
	switch(origin){
	case SEEK_SET:
		r = offset;
		break;
	case SEEK_CUR:
		if(i == prv->count){
			r = offset + prv->total;
		}else{
			r = offset + prv->info[i].offset;
			if(prv->info[i].fd >= 0){
				r += _telli64(prv->info[i].fd);
			}
		}
		break;
	case SEEK_END:
		r = offset + prv->total;
		break;
	default:
		r = offset;
	}

	if(r < 0){
		r = 0;
	}else if(r > prv->total){
		r = prv->total;
	}

	if(i < prv->count){
		if( (r < prv->info[i].offset) || (prv->info[i].offset + prv->info[i].length <= r) ){
			if(prv->info[i].fd >= 0){
				_close(prv->info[i].fd);
				prv->info[i].fd = -1;
			}
		}
	}

	for(i=0;i<prv->count;i++){
		if( (prv->info[i].offset <= r) && (r < (prv->info[i].offset + prv->info[i].length)) ){
			prv->index = i;
			if(prv->info[i].fd < 0){
				prv->info[i].fd = _open(prv->info[i].path, _O_BINARY|_O_RDONLY|_O_SEQUENTIAL);
			}
			if(prv->info[i].fd < 0){
				return prv->info[i].offset;
			}
			_lseeki64(prv->info[i].fd, r - prv->info[i].offset, SEEK_SET);
			return r;
		}
	}

	prv->index = prv->count;

	return prv->total;
}

static int mf_read(void *multi_file, void *buf, int length)
{
	int i,n,r;
	unsigned char *p;

	MULTI_FILE *mf;
	MF_PRIVATE_DATA *prv;

	mf = (MULTI_FILE *)multi_file;
	prv = (MF_PRIVATE_DATA *)mf->private_data;

	if(prv == NULL){
		return 0;
	}

	if(prv->index >= prv->count){
		return 0;
	}

	r = 0;
	p = (unsigned char *)buf;
	while(length){
		
		i = prv->index;
		
		if(prv->info[i].fd < 0){
			prv->info[i].fd = _open(prv->info[i].path, _O_BINARY|_O_RDONLY|_O_SEQUENTIAL);
		}
		if(prv->info[i].fd < 0){
			break;
		}
		
		n = _read(prv->info[i].fd, p, length);
		
		if(n == 0){
			
			_close(prv->info[i].fd);
			prv->info[i].fd = -1;
			
			i += 1;
			prv->index = i;
			
			if(i >= prv->count){
				break;
			}

		}else{
			
			r += n;	
			p += n;
			length -= n;
			
		}
	}

	return r;
}

static __int64 sf_tell(void *multi_file)
{
	MULTI_FILE *p;
	MF_PRIVATE_DATA *prv;

	p = (MULTI_FILE *)multi_file;
	prv = (MF_PRIVATE_DATA *)p->private_data;

	if(prv == NULL){
		return 0;
	}

	return _telli64(prv->info[0].fd);
}

static __int64 sf_seek(void *multi_file, __int64 offset, int origin)
{
	MULTI_FILE *p;
	MF_PRIVATE_DATA *prv;

	p = (MULTI_FILE *)multi_file;
	prv = (MF_PRIVATE_DATA *)p->private_data;

	if(prv == NULL){
		return 0;
	}

	return _lseeki64(prv->info[0].fd, offset, origin);
}

static int sf_read(void *multi_file, void *buf, int length)
{
	int n,r;
	unsigned char *p;
	MULTI_FILE *mf;
	MF_PRIVATE_DATA *prv;

	mf = (MULTI_FILE *)multi_file;
	prv = (MF_PRIVATE_DATA *)mf->private_data;

	if(prv == NULL){
		return 0;
	}

	r = 0;
	p = (unsigned char *)buf;
	while(length){
		n = _read(prv->info[0].fd, p, length);
		if(n == 0){
			break;
		}
		r += n;
		p += n;
		length -= n;
	}

	return r;
}

static int mf_count(void *multi_file)
{
	MULTI_FILE *mf;
	MF_PRIVATE_DATA *prv;

	mf = (MULTI_FILE *)multi_file;
	prv = (MF_PRIVATE_DATA *)mf->private_data;

	if(prv == NULL){
		return 0;
	}

	return prv->count;
}

static __int64 mf_border(void *multi_file, int index)
{
	MULTI_FILE *mf;
	MF_PRIVATE_DATA *prv;

	mf = (MULTI_FILE *)multi_file;
	prv = (MF_PRIVATE_DATA *)mf->private_data;

	if(prv == NULL){
		return 0;
	}

	if(index < 0){
		index = 0;
	}else if(index >= prv->count){
		index = prv->count - 1;
	}

	return prv->info[index].offset + prv->info[index].length;
}

static int try_next(char *path)
{
	char *p;
	char *suffix;
	char *base;
	char *tail;

	int need_more;

	// detect suffix
	p = path;
	suffix = NULL;
	base = NULL;
	while(*p){
		if(*p == '.'){
			suffix = p;
		}
		if(*p == '\\'){
			suffix = NULL;
			base = p+1;
		}
		p = CharNext(p);
	}

	if(suffix == NULL){
		suffix = p;
	}
	if(base == NULL){
		base = path;
	}

	// retrieve position of the last digits char
	p = base;
	tail = NULL;
	while(p < suffix){
		if(isdigit(*p)){
			tail = p;
		}
		p = CharNext(p);
	}

	if(tail == NULL){ // path has no digits char
		return -1;
	}

	// increment digits
	need_more = 1;
	p = tail;
	while(need_more){
		
		if(p < base){ /* give up */
			return -1;
		}
		if(!isdigit(*p)){ /* give up */
			return -1;
		}
			
		if(*p == '9'){
			p -= 1;
			continue;
		}

		p[0] += 1;
		while(p < tail){
			p[1] = '0';
			p += 1;
		}
		need_more = 0;
	}

	return _open(path, _O_BINARY|_O_RDONLY|_O_SEQUENTIAL);
}

