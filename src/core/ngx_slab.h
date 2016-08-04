
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_SLAB_H_INCLUDED_
#define _NGX_SLAB_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef struct ngx_slab_page_s  ngx_slab_page_t;

struct ngx_slab_page_s {
    // slabָ�������chunk��СΪ2^slab
    uintptr_t         slab;
    ngx_slab_page_t  *next;
    // �� 32 λϵͳ�ж� �ǰ� 4 �ֽڶ��� (4-byte aligned)��
    // ͬʱ�����ڴ���ʼ��ַ������ 4 �ֽڶ���ġ�
    // ���� prev��ֵ��2λ��0���������ô洢������Ϣ��
    uintptr_t         prev;
};


typedef struct {
    ngx_shmtx_sh_t    lock;
    // 2^minshit��С�����chunk��С
    size_t            min_size;
    size_t            min_shift;
    // ����
    ngx_slab_page_t  *pages;
    ngx_slab_page_t   free;
    // �ڴ�ҳ��ʼ
    u_char           *start;
    u_char           *end;

    ngx_shmtx_t       mutex;

    u_char           *log_ctx;
    u_char            zero;

    void             *data;
    void             *addr;
} ngx_slab_pool_t;


void ngx_slab_init(ngx_slab_pool_t *pool);
void *ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size);
void *ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size);
void ngx_slab_free(ngx_slab_pool_t *pool, void *p);
void ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p);


#endif /* _NGX_SLAB_H_INCLUDED_ */
