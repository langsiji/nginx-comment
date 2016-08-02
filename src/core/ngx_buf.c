
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


ngx_buf_t *
ngx_create_temp_buf(ngx_pool_t *pool, size_t size)
{
    ngx_buf_t *b;
	// start <= pos <= last <= end
    b = ngx_calloc_buf(pool);
    if (b == NULL) {
        return NULL;
    }

    b->start = ngx_palloc(pool, size);
    if (b->start == NULL) {
        return NULL;
    }

    /*
     * set by ngx_calloc_buf():
     *
     *     b->file_pos = 0;
     *     b->file_last = 0;
     *     b->file = NULL;
     *     b->shadow = NULL;
     *     b->tag = 0;
     *     and flags
     */

    b->pos = b->start;
    b->last = b->start;
    b->end = b->last + size;
    b->temporary = 1;

    return b;
}


// ��pool chain����õ����ߴ��ڴ�ط���һ��chain
ngx_chain_t *
ngx_alloc_chain_link(ngx_pool_t *pool)
{
    ngx_chain_t  *cl;

    cl = pool->chain;

    if (cl) {
        pool->chain = cl->next;
        return cl;
    }

    cl = ngx_palloc(pool, sizeof(ngx_chain_t));
	// ������ж�?
    if (cl == NULL) {
        return NULL;
    }

    return cl;
}

// ����bufs��С���ڴ�飬Ȼ��ȷֵ�ngx_buf����
// ���е�buf����������ngx_chain_t����
ngx_chain_t *
ngx_create_chain_of_bufs(ngx_pool_t *pool, ngx_bufs_t *bufs)
{
    u_char       *p;
    ngx_int_t     i;
    ngx_buf_t    *b;
    ngx_chain_t  *chain, *cl, **ll;
	// ����bufs->num * bufs->size��С���ڴ棬Ȼ��
	// ����ڴ�ȷָ�ngx_buf
    p = ngx_palloc(pool, bufs->num * bufs->size);
    if (p == NULL) {
        return NULL;
    }

    ll = &chain; // �ȷֺ��ngx_buf����������

    for (i = 0; i < bufs->num; i++) {

        b = ngx_calloc_buf(pool);
        if (b == NULL) {
            return NULL;
        }

        /*
         * set by ngx_calloc_buf():
         *
         *     b->file_pos = 0;
         *     b->file_last = 0;
         *     b->file = NULL;
         *     b->shadow = NULL;
         *     b->tag = 0;
         *     and flags
         *
         */

        b->pos = p;
        b->last = p;
        b->temporary = 1;

        b->start = p;
        p += bufs->size; //ÿ��ngx_buf��С��Ϊbufs->size
        b->end = p;

        cl = ngx_alloc_chain_link(pool);
        if (cl == NULL) {
            return NULL;
        }
		// ��������β�巨
        cl->buf = b;
        *ll = cl;
        ll = &cl->next;
    }

    *ll = NULL;

    return chain;
}

// ��in�����е�buf�ҽӵ�chain��β��
// ע�������chain��ָ���ָ�룬
// ��ʹ*chain==NULLҲ��û�й�ϵ��
ngx_int_t
ngx_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain, ngx_chain_t *in)
{
    ngx_chain_t  *cl, **ll;

    ll = chain;

    for (cl = *chain; cl; cl = cl->next) {
        ll = &cl->next;
    }

    while (in) {
        cl = ngx_alloc_chain_link(pool);
        if (cl == NULL) {
            return NGX_ERROR;
        }

        cl->buf = in->buf;
        *ll = cl;
        ll = &cl->next;
        in = in->next;
    }

    *ll = NULL;

    return NGX_OK;
}

// ��free����õ�һ��buf�ڵ�,������ڴ������
ngx_chain_t *
ngx_chain_get_free_buf(ngx_pool_t *p, ngx_chain_t **free)
{
    ngx_chain_t  *cl;

    if (*free) {
        cl = *free;
        *free = cl->next;
        cl->next = NULL;
        return cl;
    }

    cl = ngx_alloc_chain_link(p);
    if (cl == NULL) {
        return NULL;
    }

    cl->buf = ngx_calloc_buf(p);
    if (cl->buf == NULL) {
        return NULL;
    }

    cl->next = NULL;

    return cl;
}

// ��out�ϲ���busy���ͷ�busy��bufΪ0�Ľڵ㣬
// ��tag�ڵ�黹���ڴ�أ�tag�ڵ�ŵ�free����
void
ngx_chain_update_chains(ngx_pool_t *p, ngx_chain_t **free, ngx_chain_t **busy,
    ngx_chain_t **out, ngx_buf_tag_t tag)
{
    ngx_chain_t  *cl;
	// busy = busy + out
    if (*busy == NULL) {
        *busy = *out;

    } else {// ��Ϊelse if (*out) �᲻���һ��
        for (cl = *busy; cl->next; cl = cl->next) { /* void */ }

        cl->next = *out;
    }

    *out = NULL;

    while (*busy) {
        cl = *busy;
		// �ҵ�һ��buf�ڴ��С��0ֱ��������˵�������
		// ��buf���Ƿ�0
        if (ngx_buf_size(cl->buf) != 0) {
            break;
        }
		// �ͷŷ�tag�������ڴ�Ϊ0��buf
        if (cl->buf->tag != tag) {
            *busy = cl->next;
            ngx_free_chain(p, cl);
            continue;
        }
		// ����λ�ã���Ϊpos��last��һֱ����ƫ��
        cl->buf->pos = cl->buf->start;
        cl->buf->last = cl->buf->start;

		// ���ú��buf�ŵ�free����
        *busy = cl->next;
        cl->next = *free;
        *free = cl;
    }
}
