#include "instance_manager.h"

#include <stdlib.h>
#include <string.h>

#include <windows.h>

typedef struct {
	
	void             *body;
	TEARDOWN_PROC     teardown;

	void             *prev;
	void             *next;
	
} INSTANCE;

typedef struct {

	INSTANCE         *head;
	INSTANCE         *tail;

	int               count;
	
} INSTANCE_LIST;

typedef struct {

	INSTANCE_LIST     work;
	INSTANCE_LIST     pool;

	CRITICAL_SECTION  lock;
	
} INSTANCE_MANAGER;

static INSTANCE_MANAGER manager; // ファイルローカル変数で管理

static void initialize_instance_manager();
static void teardown_instance_manager();

static INSTANCE *get_head(INSTANCE_LIST *list);
static void put_tail(INSTANCE_LIST *list, INSTANCE *elem);
static INSTANCE *new_instance();
static INSTANCE *find_instance(INSTANCE_LIST *list, void *instance);
static void remove_from_list(INSTANCE_LIST *list, INSTANCE *elem);

static HINSTANCE m_hinst = NULL;

BOOL WINAPI DllMain(HINSTANCE inst, DWORD code, LPVOID reserved)
{
	// AviSynth の LoadVFAPIPlugin はファイルを閉じる際に
	// CloseFile を飛ばして FreeLibrary を呼ぶ腐れ仕様
	//
	// このため
	// 　先読みスレッド : 走り続ける
	// 　命令コード等 : 解放される
	// という齟齬が発生し、不正領域参照例外が発生する
	//
	// 腐れ仕様コードの尻を拭くため、インスタンスを内部
	// 管理して、close されずに FreeLibrary が呼ばれたら
	// 内部で close してあげる (ナンテヤサシイアテクシ)
	
	switch (code) {
	case DLL_PROCESS_ATTACH:
		initialize_instance_manager();
		m_hinst = inst;
		break;
	case DLL_PROCESS_DETACH:
		teardown_instance_manager();
		break;
	}

	return TRUE;
}

HINSTANCE get_dll_handle()
{
	return m_hinst;
}

void register_instance(void *instance, TEARDOWN_PROC teardown)
{
	INSTANCE *p;

	EnterCriticalSection(&(manager.lock));

	p = get_head(&(manager.pool));
	if (p == NULL) {
		p = new_instance();
		if (p == NULL) {
			goto LAST;
		}
	}

	p->body = instance;
	p->teardown = teardown;

	put_tail(&(manager.work), p);

LAST:
	
	LeaveCriticalSection(&(manager.lock));
	
}

void remove_instance(void *instance)
{
	INSTANCE *p;

	EnterCriticalSection(&(manager.lock));

	p = find_instance(&(manager.work), instance);
	if (p == NULL) {
		goto LAST;
	}

	remove_from_list(&(manager.work), p);

	put_tail(&(manager.pool), p);

LAST:
	LeaveCriticalSection(&(manager.lock));
}

static void initialize_instance_manager()
{
	int i;
	INSTANCE *p;
	
	memset(&manager, 0, sizeof(manager));
	InitializeCriticalSection(&(manager.lock));

	for (i=0;i<8;i++) {
		p = new_instance();
		if (p != NULL) {
			put_tail(&(manager.pool), p);
		}
	}
}

static void teardown_instance_manager()
{
	INSTANCE *p;

	while ((p = get_head(&(manager.work))) != NULL) {
		if ((p->body != NULL) && (p->teardown != NULL)) {
			p->teardown(p->body);
		}
		free(p);
	}

	while ((p = get_head(&(manager.pool))) != NULL) {
		free(p);
	}
}

static INSTANCE *get_head(INSTANCE_LIST *list)
{
	INSTANCE *elem;

	elem = list->head;
	if (elem == NULL) {
		return elem;
	}

	list->head = (INSTANCE *)(elem->next);
	if (list->head != NULL) {
		list->head->prev = NULL;
		list->count -= 1;
	} else {
		list->tail = NULL;
		list->count = 0;
	}

	elem->next = NULL;

	return elem;
}

static void put_tail(INSTANCE_LIST *list, INSTANCE *elem)
{
	if (list->tail != NULL) {
		elem->prev = list->tail;
		list->tail->next = elem;
		elem->next = NULL;
		list->tail = elem;
		list->count += 1;
	} else {
		list->head = elem;
		list->tail = elem;
		elem->prev = NULL;
		elem->next = NULL;
	}
}

static INSTANCE *new_instance()
{
	return (INSTANCE *)calloc(1, sizeof(INSTANCE));
}

static INSTANCE *find_instance(INSTANCE_LIST *list, void *instance)
{
	INSTANCE *elem = list->head;
	
	while (elem != NULL) {
		if (elem->body == instance) {
			break;
		}
		elem = (INSTANCE *)elem->next;
	}

	return elem;
}

static void remove_from_list(INSTANCE_LIST *list, INSTANCE *elem)
{
	INSTANCE *prev;
	INSTANCE *next;

	prev = (INSTANCE *)(elem->prev);
	next = (INSTANCE *)(elem->next);
	if (prev == NULL) {
		list->head = next;
	} else {
		prev->next = next;
	}

	if (next == NULL) {
		list->tail = prev;
	} else {
		next->prev = prev;
	}

	list->count -= 1;
}



