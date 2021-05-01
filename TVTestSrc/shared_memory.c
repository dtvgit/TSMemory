// by HDUSTestÇÃíÜÇÃêl
#include <stdio.h>
#include <windows.h>
#include "shared_memory.h"


typedef struct {
	void *buf;
	size_t size;
	size_t cur_pos;
} shm_info;


static const char *get_filename(const char *path)
{
	const char *filename=NULL,*p;

	p=path;
	while (*p!='\0') {
		if (*p=='\\')
			filename=p+1;
		else if (*(p+1)!='\0' && IsDBCSLeadByteEx(CP_ACP,*p))
			p++;
		p++;
	}
	if (filename==NULL)
		return path;
	return filename;
}


int open_shared_memory(const char *name)
{
	SECURITY_DESCRIPTOR sd;
	SECURITY_ATTRIBUTES sa;
	char mutex_name[MAX_PATH];
	HANDLE mutex;
	HANDLE file;
	void *src,*buf;
	DWORD size,used,pos;
	shm_info *info;

	info=malloc(sizeof(shm_info));
	if (info==NULL)
		return -1;
	memset(&sd,0,sizeof(sd));
	InitializeSecurityDescriptor(&sd,SECURITY_DESCRIPTOR_REVISION);
	SetSecurityDescriptorDacl(&sd,TRUE,NULL,FALSE);
	memset(&sa,0,sizeof(sa));
	sa.nLength=sizeof(sa);
	sa.lpSecurityDescriptor=&sd;
	name=get_filename(name);
	strcpy(mutex_name,name);
	strcat(mutex_name,".mutex");
	mutex=OpenMutexA(MUTEX_ALL_ACCESS,FALSE,mutex_name);
	if (mutex==NULL) {
		free(info);
		return -1;
	}
	if (WaitForSingleObject(mutex,3000)!=WAIT_OBJECT_0) {
		CloseHandle(mutex);
		free(info);
		return -1;
	}
	file=OpenFileMappingA(FILE_MAP_READ,FALSE,name);
	if (file==NULL) {
		ReleaseMutex(mutex);
		CloseHandle(mutex);
		free(info);
		return -1;
	}
	src=MapViewOfFile(file,FILE_MAP_READ,0,0,0);
	if (src==NULL) {
		CloseHandle(file);
		ReleaseMutex(mutex);
		CloseHandle(mutex);
		free(info);
		return -1;
	}
	size=((DWORD*)src)[0];
	used=((DWORD*)src)[1];
	pos=((DWORD*)src)[2];
	if (used>0) {
		DWORD copy_size;

		buf=malloc(used);
		if (buf==NULL) {
			UnmapViewOfFile(src);
			CloseHandle(file);
			ReleaseMutex(mutex);
			CloseHandle(mutex);
			free(info);
			return -1;
		}
		copy_size=min(used,size-pos);
		memcpy(buf,(char*)src+sizeof(DWORD)*4+pos,copy_size);
		if (copy_size<used)
			memcpy((char*)buf+copy_size,(char*)src+sizeof(DWORD)*4,used-copy_size);
	} else {
		buf=NULL;
	}
	UnmapViewOfFile(src);
	CloseHandle(file);
	ReleaseMutex(mutex);
	CloseHandle(mutex);
	info->buf=buf;
	info->size=used;
	info->cur_pos=0;
	return (int)info;
}


int shm_close(int id)
{
	shm_info *info;

	if (id==-1 || id==0)
		return 0;
	info=(shm_info*)id;
	free(info->buf);
	free(info);
	return 1;
}


int shm_read(int id,void *buf,int length)
{
	shm_info *info;

	if (id==-1 || id==0)
		return 0;
	info=(shm_info*)id;
	if (info->cur_pos+length>info->size)
		length=info->size-info->cur_pos;
	if (length<=0)
		return 0;
	memcpy(buf,(char*)info->buf+info->cur_pos,length);
	info->cur_pos+=length;
	return length;
}


__int64 shm_tell(int id)
{
	shm_info *info;

	if (id==-1 || id==0)
		return 0;
	info=(shm_info*)id;
	return (__int64)info->cur_pos;
}


__int64 shm_seek(int id,__int64 offset,int origin)
{
	shm_info *info;
	__int64 pos;

	if (id==-1 || id==0)
		return 0;
	info=(shm_info*)id;
	switch (origin) {
	case SEEK_SET:
		pos=offset;
		break;
	case SEEK_CUR:
		pos=(__int64)info->cur_pos+offset;
		break;
	case SEEK_END:
		pos=(__int64)info->size+offset;
		break;
	default:
		return 0;
	}
	if (pos<0)
		pos=0;
	else if (pos>(__int64)info->size)
		pos=(__int64)info->size;
	info->cur_pos=(size_t)pos;
	return pos;
}
