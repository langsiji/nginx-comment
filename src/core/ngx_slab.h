
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
    // 1.�������chunk<128�ֽڣ����ֶ�û�ã�bitmap���ڴ�ҳ����
    // 2.�������chunk==128�ֽڣ�slabΪ32λ���պÿ��Թ��������ڴ�ҳ��32��chunk
    // 3.�������chunk>128�ֽڣ���8λ������ʾ�����chunk��С��λ������
    // λ��������Ϊ8��9��10��11����2�ֽ���������bitmap���ο�NGX_SLAB_SHIFT_MASK��
    // ��2�ֽ�Ϊ16λ��chunk>128˵��chunk��СΪ256����Ϊchunk��С��2 ��������2k����
    // ���Ի���Ϊ16��256�ֽڵ�chunk��16bit���������ʾchunk���������ǰ����chunk��С
    // ����128byte���ο�NGX_SLAB_MAP_MASK��ʾchunk����128�ֽ����Ժ�
    // bitmap����
    
    //8λ���������洢
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
