
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_BUF_H_INCLUDED_
#define _NGX_BUF_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>


typedef void *            ngx_buf_tag_t;

typedef struct ngx_buf_s  ngx_buf_t;

struct ngx_buf_with_comment_s {
    /*
     * posͨ������������ʹ���߱���Ӧ�ô�pos���λ�ÿ�ʼ�����ڴ��е����ݣ�������������Ϊͬһ��
     * ngx_buf_t���ܱ���η�����������Ȼ��pos�ĺ�������ʹ������ģ�嶨���
     */
    u_char  *pos;
                               
    /* lastͨ����ʾ��Ч�����ݵ���Ϊֹ��ע�⣬pos��last֮����ڴ���ϣ��nginx���������� */
    u_char  *last;
                               
    /*
     * �����ļ�ʱ��file_pos��file_last�ĺ����봦���ڴ�ʱ��pos��last��ͬ��
     * file_pos��ʾ��Ҫ�������ļ�λ�ã�file_last��ʾ�������ļ�λ�á�
     */
    off_t   file_pos;
    off_t   file_last;
                               
    /* ���ngx_buf_t�����������ڴ棬��ôstartָ������ڴ����ʼ��ַ */
    u_char  *start;
                               
    /* ��start��Ա��Ӧ��ָ�򻺳����ڴ��ĩβ */
    u_char  *end;
                               
    /* ��ʾ��ǰ�����������ͣ��������ĸ�ģ��ʹ�þ�ָ�����ģ��ngx_module_t�����ĵ�ַ */
    ngx_buf_tag_t  tag;
                               
    /* ���õ��ļ� */
    ngx_file_t  *file;
                               
    /*
     * ��ǰ��������Ӱ�ӻ��������ó�Ա�����õ�����������ת�����η���������Ӧʱ��ʹ����shadow��Ա��
     * ������Ϊnginx̫��Լ�ڴ��ˣ�����һ���ڴ沢ʹ��ngx_buf_t��ʾ���յ������η�������Ӧ��
     * �������οͻ���ת��ʱ���ܻ������ڴ�洢���ļ��У�Ҳ����ֱ�������η��ͣ���ʱnginx���Բ���
     * ���¸���һ���ڴ������µ�Ŀ�ģ������ٴν���һ��ngx_buf_t�ṹ��ָ��ԭ�ڴ棬�������ngx_buf_t
     * �ṹ��ָ����ͬһ���ڴ棬����֮��Ĺ�ϵ��ͨ��shadow��Ա�����ã�һ�㲻����ʹ�á�
     */
    ngx_buf_t   *shadow;
                               
    /* ��ʱ�ڴ��־λ��Ϊ1ʱ��ʾ�������ڴ���������ڴ�����޸� */
    unsigned    temporay:1;
                               
    /* ��־λ��Ϊ1ʱ��ʾ�������ڴ���������ڴ治�����޸� */
    unsigned    memory:1;
                               
    /* ��־λ��Ϊ1ʱ��ʾ����ڴ�����nmapϵͳ����ӳ������ģ��������޸� */
    unsigned    mmap:1;
                               
    /* ��־λ��Ϊ1ʱ��ʾ�ɻ��� */
    unsigned    recycled:1;
                               
    /* ��־λ��Ϊ1ʱ��ʾ��λ��������������ļ��������ڴ� */
    unsigned    in_file:1;
                               
    /* ��־λ��Ϊ1ʱ��ʾ��Ҫִ��flush���� */
    unsigned    flush:1;
                               
    /*
     * ��־λ�����ڲ�����黺����ʱ�Ƿ�ʹ��ͬ����ʽ����������ǣ�����ܻ�����nginx���̣�nginx������
     * �������������첽�ģ�������֧�ָ߲����Ĺؼ�����Щ��ܴ�����syncΪ1ʱ���ܻ��������ķ�ʽ����I/O
     * ����������������ʹ������nginxģ�������
     */
    unsigned    sync:1;
                               
    /*
     * ��־λ����ʾ�Ƿ������һ�黺��������Ϊngx_buf_t������ngx_chain_t�����������������Ϊ1ʱ��
     * ��ʾ��ǰ�����һ��������Ļ�������   
     */
    unsigned    last_buf:1;
                               
    /* ��־λ����ʾ�Ƿ���ngx_chain_t�е����һ�黺���� */
    unsigned    last_in_chain:1;
                               
    /* ��־λ����ʾ�Ƿ������һ��Ӱ�ӻ���������shadow�����ʹ�á�ͨ��������ʹ���� */
    unsigned    last_shadow:1;
                               
    /* ��־λ����ʾ��ǰ�������Ƿ�������ʱ�ļ� */
    unsigned    temp_file:1;
}


struct ngx_buf_s {
    u_char          *pos;
    u_char          *last;
    off_t            file_pos;
    off_t            file_last;

    u_char          *start;         /* start of buffer */
    u_char          *end;           /* end of buffer */
    ngx_buf_tag_t    tag;
    ngx_file_t      *file;
    ngx_buf_t       *shadow;


    /* the buf's content could be changed */
    unsigned         temporary:1;

    /*
     * the buf's content is in a memory cache or in a read only memory
     * and must not be changed
     */
    unsigned         memory:1;

    /* the buf's content is mmap()ed and must not be changed */
    unsigned         mmap:1;

    unsigned         recycled:1;
    unsigned         in_file:1;
    unsigned         flush:1;
    unsigned         sync:1;
    unsigned         last_buf:1;
    unsigned         last_in_chain:1;

    unsigned         last_shadow:1;
    unsigned         temp_file:1;

    /* STUB */ int   num;
};


struct ngx_chain_s {
    ngx_buf_t    *buf;
    ngx_chain_t  *next;
};


typedef struct {
    ngx_int_t    num;
    size_t       size;
} ngx_bufs_t;


typedef struct ngx_output_chain_ctx_s  ngx_output_chain_ctx_t;

typedef ngx_int_t (*ngx_output_chain_filter_pt)(void *ctx, ngx_chain_t *in);

#if (NGX_HAVE_FILE_AIO)
typedef void (*ngx_output_chain_aio_pt)(ngx_output_chain_ctx_t *ctx,
    ngx_file_t *file);
#endif

struct ngx_output_chain_ctx_s {
    ngx_buf_t                   *buf;
    ngx_chain_t                 *in;
    ngx_chain_t                 *free;
    ngx_chain_t                 *busy;

    unsigned                     sendfile:1;
    unsigned                     directio:1;
#if (NGX_HAVE_ALIGNED_DIRECTIO)
    unsigned                     unaligned:1;
#endif
    unsigned                     need_in_memory:1;
    unsigned                     need_in_temp:1;
#if (NGX_HAVE_FILE_AIO)
    unsigned                     aio:1;

    ngx_output_chain_aio_pt      aio_handler;
#endif

    off_t                        alignment;

    ngx_pool_t                  *pool;
    ngx_int_t                    allocated;
    ngx_bufs_t                   bufs;
    ngx_buf_tag_t                tag;

    ngx_output_chain_filter_pt   output_filter;
    void                        *filter_ctx;
};


typedef struct {
    ngx_chain_t                 *out;
    ngx_chain_t                **last;
    ngx_connection_t            *connection;
    ngx_pool_t                  *pool;
    off_t                        limit;
} ngx_chain_writer_ctx_t;


#define NGX_CHAIN_ERROR     (ngx_chain_t *) NGX_ERROR


#define ngx_buf_in_memory(b)        (b->temporary || b->memory || b->mmap)
#define ngx_buf_in_memory_only(b)   (ngx_buf_in_memory(b) && !b->in_file)

#define ngx_buf_special(b)                                                   \
    ((b->flush || b->last_buf || b->sync)                                    \
     && !ngx_buf_in_memory(b) && !b->in_file)

#define ngx_buf_sync_only(b)                                                 \
    (b->sync                                                                 \
     && !ngx_buf_in_memory(b) && !b->in_file && !b->flush && !b->last_buf)

#define ngx_buf_size(b)                                                      \
    (ngx_buf_in_memory(b) ? (off_t) (b->last - b->pos):                      \
                            (b->file_last - b->file_pos))

ngx_buf_t *ngx_create_temp_buf(ngx_pool_t *pool, size_t size);
ngx_chain_t *ngx_create_chain_of_bufs(ngx_pool_t *pool, ngx_bufs_t *bufs);


#define ngx_alloc_buf(pool)  ngx_palloc(pool, sizeof(ngx_buf_t))
#define ngx_calloc_buf(pool) ngx_pcalloc(pool, sizeof(ngx_buf_t))

ngx_chain_t *ngx_alloc_chain_link(ngx_pool_t *pool);
#define ngx_free_chain(pool, cl)                                             \
    cl->next = pool->chain;                                                  \
    pool->chain = cl



ngx_int_t ngx_output_chain(ngx_output_chain_ctx_t *ctx, ngx_chain_t *in);
ngx_int_t ngx_chain_writer(void *ctx, ngx_chain_t *in);

ngx_int_t ngx_chain_add_copy(ngx_pool_t *pool, ngx_chain_t **chain,
    ngx_chain_t *in);
ngx_chain_t *ngx_chain_get_free_buf(ngx_pool_t *p, ngx_chain_t **free);
void ngx_chain_update_chains(ngx_pool_t *p, ngx_chain_t **free,
    ngx_chain_t **busy, ngx_chain_t **out, ngx_buf_tag_t tag);


#endif /* _NGX_BUF_H_INCLUDED_ */