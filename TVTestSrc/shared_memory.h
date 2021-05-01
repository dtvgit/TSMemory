// by HDUSTest‚Ì’†‚Ìl
#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif

int open_shared_memory(const char *name);
int shm_close(int id);
int shm_read(int id,void *buf,int length);
__int64 shm_tell(int id);
__int64 shm_seek(int id,__int64 offset,int origin);

#ifdef __cplusplus
}
#endif

#endif
