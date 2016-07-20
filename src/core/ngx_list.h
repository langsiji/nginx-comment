
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_LIST_H_INCLUDED_
#define _NGX_LIST_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

typedef struct ngx_list_part_s  ngx_list_part_t;

struct ngx_list_part_s {
    void             *elts; 	//ָ��һ���������ڴ棬��n��Ԫ��
    ngx_uint_t        nelts; 	//elts�Ѿ�ʹ���˶��ٸ�Ԫ��,��elts==n���´���һ������ڵ�
    ngx_list_part_t  *next; 	//��һ������ڵ�
};


/*
ÿ������ڵ����һ������,ÿ�η���һ�������Ԫ�ظ��û���
Ԫ�ش�С�Լ�����Ԫ�ظ������û�ָ����������Ԫ��ʹ����
֮��ͻ��´���һ������ڵ�


�����ʼ����ʱ��Ĳ���:
1. size ���������ȥ��Ԫ�ص��ڴ��С,��ngx_list_part_s::elts��һ��Ԫ�صĴ�С
2. n ��ʾngx_list_part_s::elts���ж��ٸ�Ԫ��
3. pool���ڷ����ڴ���ڴ��







last: ָ�����������һ���ڵ�,���ڷ���ڵ��ȥ

part: ��������׸���ž���Ԫ�صĽڵ�

size: �����д�ŵľ���Ԫ�������ڴ��С,ngx_list_part_s::eltsԪ�ش�С

nalloc: ÿ������ڵ�������Ԫ�ص�����,nalloc >= ngx_list_part_s::nelts

pool: ��listʹ�õķ����ڴ��pool

*/

typedef struct {
    ngx_list_part_t  *last;
    ngx_list_part_t   part;
    size_t            size; // Ԫ�ش�С
    ngx_uint_t        nalloc;
    ngx_pool_t       *pool;
} ngx_list_t;

// ����ÿ���ڵ㶼��һ�����飬Ԫ�ش�������鵱��
ngx_list_t *ngx_list_create(ngx_pool_t *pool, ngx_uint_t n, size_t size);

// n ����ڵ������С
// size Ԫ�ش�С
static ngx_inline ngx_int_t
ngx_list_init(ngx_list_t *list, ngx_pool_t *pool, ngx_uint_t n, size_t size)
{
    list->part.elts = ngx_palloc(pool, n * size);
    if (list->part.elts == NULL) {
        return NGX_ERROR;
    }

    list->part.nelts = 0;
    list->part.next = NULL;
    list->last = &list->part;
    list->size = size;
    list->nalloc = n;
    list->pool = pool;

    return NGX_OK;
}


/*
 *
 *  the iteration through the list:
 *
 *  part = &list.part;
 *  data = part->elts;
 *
 *  for (i = 0 ;; i++) {
 *
 *      if (i >= part->nelts) {
 *          if (part->next == NULL) {
 *              break;
 *          }
 *
 *          part = part->next;
 *          data = part->elts;
 *          i = 0;
 *      }
 *
 *      ...  data[i] ...
 *
 *  }
 */


void *ngx_list_push(ngx_list_t *list);


#endif /* _NGX_LIST_H_INCLUDED_ */
