#ifndef INSTANCE_MANAGER_H
#define INSTANCE_MANAGER_H

typedef void (* TEARDOWN_PROC)(void *);

#ifdef __cplusplus
extern "C" {
#endif

extern void register_instance(void *instance, TEARDOWN_PROC teardown);
extern void remove_instance(void *instance);

#ifdef __cplusplus
}
#endif

#endif /* INSTANCE_MANAGER_H */
