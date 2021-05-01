/*******************************************************************
                    Output Buffer Module
 *******************************************************************/
#include <stdlib.h>
#define OUT_BUFFER_C
#include "out_buffer.h"

/* global */
void init_out_buffer(OUT_BUFFER *p);
void reset_out_buffer(OUT_BUFFER *p);
void release_out_buffer(OUT_BUFFER *p);

OUT_BUFFER_ELEMENT *query_work_frame(OUT_BUFFER *p, int width, int height);
void back_work_frame(OUT_BUFFER *p, OUT_BUFFER_ELEMENT *elem);
void set_decoded_frame(OUT_BUFFER *p, OUT_BUFFER_ELEMENT *elem);
OUT_BUFFER_ELEMENT *search_out_buffer(OUT_BUFFER *p, __int64 index);

/* local */
static void put_list_head(OUT_BUFFER_LIST *list, OUT_BUFFER_ELEMENT *elem);
static void put_list_tail(OUT_BUFFER_LIST *list, OUT_BUFFER_ELEMENT *elem);
static OUT_BUFFER_ELEMENT *get_list_head(OUT_BUFFER_LIST *list);
static void insert_list_element(OUT_BUFFER_LIST *list, OUT_BUFFER_ELEMENT *elem);
static OUT_BUFFER_ELEMENT *remove_out_buffer_cache(OUT_BUFFER *p, OUT_BUFFER_ELEMENT *elem);

/* global implementation */
void init_out_buffer(OUT_BUFFER *p)
{
	memset(p, 0, sizeof(OUT_BUFFER));
	InitializeCriticalSection(&(p->lock));
}

void reset_out_buffer(OUT_BUFFER *p)
{
	OUT_BUFFER_ELEMENT *elem;

	EnterCriticalSection(&(p->lock));

	while ((elem = get_list_head(&(p->cache))) != NULL) {
		
		memset(&(elem->prm), 0, sizeof(OUTPUT_PARAMETER));
		put_list_tail(&(p->pool), elem);

	}

	p->i_frame_count = 0;

	while ((elem = get_list_head(&(p->dec))) != NULL) {

		memset(&(elem->prm), 0, sizeof(OUTPUT_PARAMETER));
		put_list_tail(&(p->pool), elem);

	}

	LeaveCriticalSection(&(p->lock));
}

void release_out_buffer(OUT_BUFFER *p)
{
	OUT_BUFFER_ELEMENT *elem;
	
	EnterCriticalSection(&(p->lock));

	while ((elem = get_list_head(&(p->pool))) != NULL) {
		free(elem);
	}

	while ((elem = get_list_head(&(p->cache))) != NULL) {
		free(elem);
	}

	while ((elem = get_list_head(&(p->dec))) != NULL) {
		free(elem);
	}

	LeaveCriticalSection(&(p->lock));

	DeleteCriticalSection(&(p->lock));
}

OUT_BUFFER_ELEMENT *query_work_frame(OUT_BUFFER *p, int width, int height)
{
	int n;
	unsigned char *ptr;
	OUT_BUFFER_ELEMENT *r;

	r = NULL;

	EnterCriticalSection(&(p->lock));

	while ((r = get_list_head(&(p->pool))) != NULL) {
		if ((r->data.width == width) && (r->data.height == height)) {
			/* use found element */
			goto LAST;
		}
		/* incompatible element */
		free(r);
	}

	/* no compatible element - create new element */
	n = sizeof(OUT_BUFFER_ELEMENT) + (width * height * 3 + 16);
	r = (OUT_BUFFER_ELEMENT *)calloc(1, n);
	if (r == NULL) {
		/* error - no enough memory */
		goto LAST;
	}

	r->data.width = width;
	r->data.height = height;

	ptr = (unsigned char *) (r + 1);
	r->data.y = (ptr + 16) - (((long)ptr) & 0x0f);
	r->data.u = r->data.y + (width * height);
	r->data.v = r->data.u + (width * height);
	
LAST:
	LeaveCriticalSection(&(p->lock));

	if (r != NULL) {
		r->ref_count = 1;
	}

	return r;
}

void back_work_frame(OUT_BUFFER *p, OUT_BUFFER_ELEMENT *elem)
{
	EnterCriticalSection(&(p->lock));

	elem->ref_count -= 1;
	if (elem->ref_count < 1) {
		put_list_tail(&(p->pool), elem);
	}

	LeaveCriticalSection(&(p->lock));
}

void set_decoded_frame(OUT_BUFFER *p, OUT_BUFFER_ELEMENT *elem)
{
	EnterCriticalSection(&(p->lock));
	
	elem->ref_count += 1;
	insert_list_element(&(p->dec), elem);

	LeaveCriticalSection(&(p->lock));
}

OUT_BUFFER_ELEMENT *search_out_buffer(OUT_BUFFER *p, __int64 index)
{
	OUT_BUFFER_ELEMENT *r;
	OUT_BUFFER_ELEMENT *cur;

	static const int min_buffer_size = 5;
	static const int max_buffer_size = 20;

	r = NULL;
	cur = NULL;
	
	EnterCriticalSection(&(p->lock));

	/* feed from decode buffer */
	while ((cur = get_list_head(&(p->dec))) != NULL) {
		
		if (cur->prm.index > index) {
			put_list_head(&(p->dec), cur);
			break;
		}

		insert_list_element(&(p->cache), cur);
		if (cur->prm.picture_coding_type == 1) {
			// I picture
			p->i_frame_count += 1;
		}

		if (cur->prm.index == index) {
			r = cur;

		}
	}

	/* remove head - unused */
	cur = p->cache.head; 
	while ((cur != NULL) && (p->cache.count > min_buffer_size)) {

		if (cur->prm.index == index) {
			r = cur;
			break;
		}

		if (cur->prm.index == (index-1)) {
			/* keep previous frame (for swap field order) */
			cur = cur->next;
			continue;
		}

		if (cur->prm.index > index) {
			break;
		}

		if (p->cache.count > max_buffer_size) {
			cur = remove_out_buffer_cache(p, cur);
			continue;
		}

		if (p->i_frame_count > 1) {
			cur = remove_out_buffer_cache(p, cur);
			continue;
		}

		break;
	}

	/* search from tail */
	cur = p->cache.tail;
	while ((r == NULL) && (cur != NULL)) {
		
		if (cur->prm.index == index) {
			r = cur;
			break;
		}

		if (cur->prm.index < index) {
			break;
		}

		cur = (OUT_BUFFER_ELEMENT *)(cur->prev);
	}

LAST:
	LeaveCriticalSection(&(p->lock));

	return r;
}

/* local implementation */
static void put_list_head(OUT_BUFFER_LIST *list, OUT_BUFFER_ELEMENT *elem)
{
	if (list->head != NULL) {
		elem->prev = NULL;
		elem->next = list->head;
		list->head->prev = elem;
		list->head = elem;
		list->count += 1;
	} else {
		elem->prev = NULL;
		elem->next = NULL;
		list->head = elem;
		list->tail = elem;
		list->count = 1;
	}
}

static void put_list_tail(OUT_BUFFER_LIST *list, OUT_BUFFER_ELEMENT *elem)
{
	if (list->tail != NULL) {
		list->tail->next = elem;
		elem->prev = list->tail;
		list->tail = elem;
		elem->next = NULL;
		list->count += 1;
	} else {
		list->head = elem;
		list->tail = elem;
		elem->prev = NULL;
		elem->next = NULL;
		list->count = 1;
	}
}

static OUT_BUFFER_ELEMENT *get_list_head(OUT_BUFFER_LIST *list)
{
	OUT_BUFFER_ELEMENT *r;
	
	if (list->head == NULL) {
		return NULL;
	}

	r = list->head;
	if (r->next != NULL) {
		list->head = (OUT_BUFFER_ELEMENT *)(r->next);
		list->head->prev = NULL;
		r->prev = NULL;
		r->next = NULL;
		list->count -= 1;
	} else {
		list->head = NULL;
		list->tail = NULL;
		r->prev = NULL;
		r->next = NULL;
		list->count = 0;
	}

	return r;
}

static void insert_list_element(OUT_BUFFER_LIST *list, OUT_BUFFER_ELEMENT *elem)
{
	OUT_BUFFER_ELEMENT *cur;
	OUT_BUFFER_ELEMENT *nxt;

	if (list->tail == NULL) {
		put_list_tail(list, elem);
		return;
	}

	cur = list->tail;
	while (cur != NULL) {
		if (cur->prm.index > elem->prm.index) {
			cur = (OUT_BUFFER_ELEMENT *)(cur->prev);
			continue;
		}

		nxt = (OUT_BUFFER_ELEMENT *)(cur->next);
		if (nxt == NULL) {
			cur->next = elem;
			elem->prev = cur;
			elem->next = NULL;
			list->tail = elem;
			list->count += 1;
			return;
		} else {
			cur->next = elem;
			elem->prev = cur;
			elem->next = nxt;
			nxt->prev = elem;
			list->count += 1;
			return;
		}
	}
		
	put_list_head(list, elem);
	return;
}

static OUT_BUFFER_ELEMENT *remove_out_buffer_cache(OUT_BUFFER *p, OUT_BUFFER_ELEMENT *elem)
{
	OUT_BUFFER_ELEMENT *pre;
	OUT_BUFFER_ELEMENT *nxt;

	pre = (OUT_BUFFER_ELEMENT *)(elem->prev);
	nxt = (OUT_BUFFER_ELEMENT *)(elem->next);

	if (elem->ref_count > 1) {
		/* do nothing */
		return nxt;
	}

	elem->ref_count -= 1;
	p->cache.count -= 1;

	elem->prev = NULL;
	elem->next = NULL;
	
	if (pre != NULL) {
		pre->next = nxt;
	} else {
		p->cache.head = nxt;
	}
	
	if (nxt != NULL) {
		nxt->prev = pre;
	} else {
		p->cache.tail = pre;
	}

	if (elem->prm.picture_coding_type == 1) {
		p->i_frame_count -= 1;
	}

	memset(&(elem->prm), 0, sizeof(OUTPUT_PARAMETER));
	elem->ref_count = 0;

	put_list_tail(&(p->pool), elem);

	return nxt;
}
