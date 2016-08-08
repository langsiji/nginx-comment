
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */

#include <ngx_config.h>
#include <ngx_core.h>


#define NGX_SLAB_PAGE_MASK   3
#define NGX_SLAB_PAGE        0
#define NGX_SLAB_BIG         1
#define NGX_SLAB_EXACT       2
#define NGX_SLAB_SMALL       3

#if (NGX_PTR_SIZE == 4)

#define NGX_SLAB_PAGE_FREE   0
#define NGX_SLAB_PAGE_BUSY   0xffffffff
#define NGX_SLAB_PAGE_START  0x80000000

#define NGX_SLAB_SHIFT_MASK  0x0000000f
#define NGX_SLAB_MAP_MASK    0xffff0000
#define NGX_SLAB_MAP_SHIFT   16

#define NGX_SLAB_BUSY        0xffffffff

#else /* (NGX_PTR_SIZE == 8) */

#define NGX_SLAB_PAGE_FREE   0
#define NGX_SLAB_PAGE_BUSY   0xffffffffffffffff
#define NGX_SLAB_PAGE_START  0x8000000000000000

#define NGX_SLAB_SHIFT_MASK  0x000000000000000f
#define NGX_SLAB_MAP_MASK    0xffffffff00000000
#define NGX_SLAB_MAP_SHIFT   32

#define NGX_SLAB_BUSY        0xffffffffffffffff

#endif


#if (NGX_DEBUG_MALLOC)

#define ngx_slab_junk(p, size)     ngx_memset(p, 0xA5, size)

#elif (NGX_HAVE_DEBUG_MALLOC)

#define ngx_slab_junk(p, size)                                                \
    if (ngx_debug_malloc)          ngx_memset(p, 0xA5, size)

#else

#define ngx_slab_junk(p, size)

#endif

static ngx_slab_page_t *ngx_slab_alloc_pages(ngx_slab_pool_t *pool,
    ngx_uint_t pages);
static void ngx_slab_free_pages(ngx_slab_pool_t *pool, ngx_slab_page_t *page,
    ngx_uint_t pages);
static void ngx_slab_error(ngx_slab_pool_t *pool, ngx_uint_t level,
    char *text);

// 2k
static ngx_uint_t  ngx_slab_max_size;
// 128byte
static ngx_uint_t  ngx_slab_exact_size;
// 32λΪ7
static ngx_uint_t  ngx_slab_exact_shift;
// ngx_slab��memcached��slab alloc���ƻ���һ��

void
ngx_slab_init(ngx_slab_pool_t *pool)
{
    u_char           *p;
    size_t            size;
    ngx_int_t         m;
    ngx_uint_t        i, n, pages;
    ngx_slab_page_t  *slots;

    /* STUB */
    // �� 32 λϵͳ pagesize Ϊ 4K �Ļ�����
    // ngx_slab_max_size ֵ�� 2K
    if (ngx_slab_max_size == 0) {
            /*
            һ�� uintptr_t ���͵ı�����Ϊ bitmap ��ʹ��ʱ��
            ������������ 8 * sizeof(uintptr_t) �� chunk ��ʹ��״̬��
            ���ոպá� ������һ�� uintptr_t �ռ�� bitmap �ܹ���
            ��һ�� slab �����е� chunk ʱ��chunk �Ĵ�С���� ngx_slab_exact_size��
            ������ slot �� shift ֵ���� ngx_slab_exact_shift�� �� 32λϵͳ��
            pagesize Ϊ 4K �Ļ����£�ngx_slab_exact_size ֵΪ 128B�� 
            ngx_slab_exact_shift ֵΪ 7��        */
        ngx_slab_max_size = ngx_pagesize / 2;
       //һҳ�ֳɶ����,��ĳ������Ҫ����Ƿ񱻷���,
       //��һҳ�ռ����ñ��ֳ�32��128�ֽڴ�С�Ŀ飬
       //�������ǿ�����һ��int��32λ��ʾ����ʹ�������
       //����ʱ,������ʹ��ngx_slab_page_s�ṹ���е�slab��Ա����ʾ���ʹ�������

        ngx_slab_exact_size = ngx_pagesize / (8 * sizeof(uintptr_t));
		// ngx_slab_exact_size���λ��
        for (n = ngx_slab_exact_size; n >>= 1; ngx_slab_exact_shift++) {
            /* void */
        }
    }
    /**/
	// �ڴ�ṹngx_slab_pool_t/slots/pages
    pool->min_size = 1 << pool->min_shift;

    p = (u_char *) pool + sizeof(ngx_slab_pool_t);
    size = pool->end - p;

    ngx_slab_junk(p, size);

    slots = (ngx_slab_page_t *) p;
    n = ngx_pagesize_shift - pool->min_shift;

    for (i = 0; i < n; i++) {
        slots[i].slab = 0;
        slots[i].next = &slots[i];	// ָ���Լ���������
        slots[i].prev = 0;
    }

    p += n * sizeof(ngx_slab_page_t);
	// ÿ��page��Сngx_pagesize + sizeof(ngx_slab_page_t)��ǰ��洢n*ngx_slab_page_t���飬
	// ����洢n*ngx_pagesizeҳ�ڴ�
    pages = (ngx_uint_t) (size / (ngx_pagesize + sizeof(ngx_slab_page_t)));
    // ��ʼ��ǰ���ngx_slab_page_t���飬�����Щ���鶼��
    // �ҽӵ�slots
    ngx_memzero(p, pages * sizeof(ngx_slab_page_t));

    pool->pages = (ngx_slab_page_t *) p;

    pool->free.prev = 0;
    pool->free.next = (ngx_slab_page_t *) p;

    pool->pages->slab = pages;
    pool->pages->next = &pool->free;
    pool->pages->prev = (uintptr_t) &pool->free;

    // �ֽڶ��룬startָ���ڴ�ҳ
    pool->start = (u_char *)
                  ngx_align_ptr((uintptr_t) p + pages * sizeof(ngx_slab_page_t),
                                 ngx_pagesize);
	// ȷ���Ƿ�Ҫ����ָ�룬��Ϊ���һҳ���ڴ���ܲ���һҳ
    m = pages - (pool->end - pool->start) / ngx_pagesize;
    if (m > 0) {
        pages -= m;
        pool->pages->slab = pages;
    }

    pool->log_ctx = &pool->zero;
    pool->zero = '\0';
}


void *
ngx_slab_alloc(ngx_slab_pool_t *pool, size_t size)
{
    void  *p;

    ngx_shmtx_lock(&pool->mutex);

    p = ngx_slab_alloc_locked(pool, size);

    ngx_shmtx_unlock(&pool->mutex);

    return p;
}


void *
ngx_slab_alloc_locked(ngx_slab_pool_t *pool, size_t size)
{
    size_t            s;
    uintptr_t         p, n, m, mask, *bitmap;
    ngx_uint_t        i, slot, shift, map;
    ngx_slab_page_t  *page, *prev, *slots;
    // �������ֵ������slab����
    if (size >= ngx_slab_max_size) {

        ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, ngx_cycle->log, 0,
                       "slab alloc: %uz", size);

        page = ngx_slab_alloc_pages(pool, (size >> ngx_pagesize_shift)
                                          + ((size % ngx_pagesize) ? 1 : 0));
        if (page) {
            p = (page - pool->pages) << ngx_pagesize_shift;
            p += (uintptr_t) pool->start;

        } else {
            p = 0;
        }

        goto done;
    }
    // ����õ�Ӧ�����Ǹ�slot�����ڴ棬
    // ��slot�����chunk��С�պô��ڵ���size
    // 
    if (size > pool->min_size) {
        shift = 1;
        for (s = size - 1; s >>= 1; shift++) { /* void */ }
        slot = shift - pool->min_shift;

    } else {
        size = pool->min_size;
        shift = pool->min_shift;
        slot = 0;
    }
    // ע��: 2^shift ����1<<shift��ʾ����chunk��С

    // ��������п���(xxx) << shift��ʾ����(xxx)*chunk_size
    ngx_log_debug2(NGX_LOG_DEBUG_ALLOC, ngx_cycle->log, 0,
                   "slab alloc: %uz slot: %ui", size, slot);

    slots = (ngx_slab_page_t *) ((u_char *) pool + sizeof(ngx_slab_pool_t));
    page = slots[slot].next;
     // �Ƿ��п��е�ҳ�ڴ�    
    if (page->next != page) {
        if (shift < ngx_slab_exact_shift) {
            // ������ڴ�С��128byte
            // һ��32λ��slab������Ϊbitmap��128*32=4k
            //��Ϊshift<ngx_slab_exact_shift��
            //bitmap��Ȼ���Ĵ���1��uintptr_t���ͣ�Ҳ��map������2
            do {
                // page - pool->pagesΪ���ڷ����slab��slab�����λ��
                // һ��page�Ĵ�СΪ1<<ngx_pagesize_shift����ôp����
                // ��Ӧ���ڴ�ҳ�����ʼλ�õ�ƫ��
                p = (page - pool->pages) << ngx_pagesize_shift;//�൱�ڳ����ڴ�ҳ��С
                // bitmapָ������ڷ����ڴ��slab������ڴ�ҳ
                bitmap = (uintptr_t *) (pool->start + p);

                // 1 << (ngx_pagesize_shift - shift) ��ʾ��slot���Է�����ٸ�chunk
                // 1<<shift��ʾ��slot��chunk��С��1<<ngx_pagesize_shift��ʾpage��С
                // chunk����Ϊ2^ngx_pagesize_shift  / 2^shift  = 2^(ngx_pagesize_shift-shift)
                map = (1 << (ngx_pagesize_shift - shift))
                          / (sizeof(uintptr_t) * 8);
                //map��ʾ��Ҫ���ٸ�bitmap��ӳ��������е�chunk
                for (n = 0; n < map; n++) {

                    if (bitmap[n] != NGX_SLAB_BUSY) {
                        //bitmap����λ������0��˵���п����chunk
                        for (m = 1, i = 0; m; m <<= 1, i++) {
                            if ((bitmap[n] & m)) {
                                continue;
                            }
                            
                            bitmap[n] |= m;
                            // ����������iλΪ0

                            
                            // �õ���Ӧchunk��λ�ã������ڴ�ҳ
                            // ��n*32+i��chunk
                            i = ((n * sizeof(uintptr_t) * 8) << shift)
                                + (i << shift);

                            if (bitmap[n] == NGX_SLAB_BUSY) {
                                for (n = n + 1; n < map; n++) {
                                     if (bitmap[n] != NGX_SLAB_BUSY) {
                                        // ���п����chunk
                                         p = (uintptr_t) bitmap + i;

                                         goto done;
                                     }
                                }
                                // ������˵�����ڴ�ҳ��chunk�Ѿ�ȫ������
                                prev = (ngx_slab_page_t *)
                                            (page->prev & ~NGX_SLAB_PAGE_MASK);//#define NGX_SLAB_PAGE_MASK   3
                                prev->next = page->next;
                                page->next->prev = page->prev;

                                page->next = NULL;
                                page->prev = NGX_SLAB_SMALL;
                            }

                            p = (uintptr_t) bitmap + i;

                            goto done;
                        }
                    }
                }

                page = page->next;

            } while (page);

        } else if (shift == ngx_slab_exact_shift) {
            // 128byte���պÿ�����page->slab��Ϊbitmap��32λ
            // pagesize=4k  pagesize/32=128
            do {
                if (page->slab != NGX_SLAB_BUSY) {

                    for (m = 1, i = 0; m; m <<= 1, i++) {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;

                        if (page->slab == NGX_SLAB_BUSY) {
                            prev = (ngx_slab_page_t *)
                                            (page->prev & ~NGX_SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = NGX_SLAB_EXACT;
                        }

                        p = (page - pool->pages) << ngx_pagesize_shift; // �ڼ�ҳ
                        p += i << shift;    // ҳ��ƫ��
                        p += (uintptr_t) pool->start;  // chunk��ַ

                        goto done;
                    }
                }

                page = page->next;

            } while (page);

        } else { /* shift > ngx_slab_exact_shift */
            //��λ���ֱ�Ϊ8��9��10��11��Щ�����������Ϊ12
            //��Ϊ2^12==2k==ngx_slab_max_size

            
            //�����һ��ҳ���Էֳɶ��ٸ�chunk
            // ����chunk��СΪ256byte��page->slab & NGX_SLAB_SHIFT_MASK����8
            // 32λngx_pagesize_shiftΪ12����2k�����Եõ�nΪ4��1<<n����16
            // Ҳ����˵��ʱ���ڴ�ҳ���Է���Ϊ16��chunk����С��Ϊ256byte
            n = ngx_pagesize_shift - (page->slab & NGX_SLAB_SHIFT_MASK);
            n = 1 << n; 

            // �õ���ʾ��Щ�����������bitmap����������chunk��С256byte
            // ����n����16λ
            n = ((uintptr_t) 1 << n) - 1;
            mask = n << NGX_SLAB_MAP_SHIFT; // 32λϵͳNGX_SLAB_MAP_SHIFTΪ16

            do {
                if ((page->slab & NGX_SLAB_MAP_MASK) != mask) { // ��ҳ����δ�����chunk

                    for (m = (uintptr_t) 1 << NGX_SLAB_MAP_SHIFT, i = 0;
                         m & mask;
                         m <<= 1, i++)
                    {
                        if ((page->slab & m)) {
                            continue;
                        }

                        page->slab |= m;

                        if ((page->slab & NGX_SLAB_MAP_MASK) == mask) {
                            // ���ڴ�ҳ��chunk������
                            prev = (ngx_slab_page_t *)
                                            (page->prev & ~NGX_SLAB_PAGE_MASK);
                            prev->next = page->next;
                            page->next->prev = page->prev;

                            page->next = NULL;
                            page->prev = NGX_SLAB_BIG;
                        }
                        // ����chunk��λ��
                        p = (page - pool->pages) << ngx_pagesize_shift;
                        p += i << shift;
                        p += (uintptr_t) pool->start;

                        goto done;
                    }
                }

                page = page->next;

            } while (page);
        }
    }
    // û�п��õ��ڴ�ҳ��slab���䣬�������һ���ڴ�ҳ
    // �ҽӵ���Ӧ��slab����chunk�����ȥ
    page = ngx_slab_alloc_pages(pool, 1);

    if (page) {
        if (shift < ngx_slab_exact_shift) {
            // ��Ҫ��n��chunk������bitmap
            p = (page - pool->pages) << ngx_pagesize_shift;
            bitmap = (uintptr_t *) (pool->start + p);

            s = 1 << shift; // chunk_size
            // ����������˼��:
            // chunk_count = page_size/chunk_size
            // map_bytes = chunk_count/8
            // bitmap_chunks = map_bytes / chunk_size
            // n = bitmap_chunks
            n = (1 << (ngx_pagesize_shift - shift)) / 8 / s; // n��ʾ��Ҫ���ڴ洢bitmap��chunk����

            if (n == 0) {
                n = 1;
            }
            // ����Ϊ2����ΪҪ����һ��chunk��������
            // ��Ϊbitmap[0] = (1 << (n+1)) - 1;�Ƚ��ʺ��˿�
            
            bitmap[0] = (2 << n) - 1; // ��עn��chunk�Ѿ�����������bitmapʹ��

            map = (1 << (ngx_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

            for (i = 1; i < map; i++) {
                bitmap[i] = 0;
            }

            // �����䵽��page�����Ӧ��slot��
            page->slab = shift;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_SMALL;

            slots[slot].next = page;

            // �õ������ȥ��chunk��ַ
            p = ((page - pool->pages) << ngx_pagesize_shift) + s * n;
            p += (uintptr_t) pool->start;

            goto done;

        } else if (shift == ngx_slab_exact_shift) {

            page->slab = 1;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_EXACT;

            slots[slot].next = page;

            p = (page - pool->pages) << ngx_pagesize_shift;
            p += (uintptr_t) pool->start;

            goto done;

        } else { /* shift > ngx_slab_exact_shift */

            page->slab = ((uintptr_t) 1 << NGX_SLAB_MAP_SHIFT) | shift;
            page->next = &slots[slot];
            page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_BIG;

            slots[slot].next = page;

            p = (page - pool->pages) << ngx_pagesize_shift;
            p += (uintptr_t) pool->start;

            goto done;
        }
    }

    p = 0;

done:

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, ngx_cycle->log, 0, "slab alloc: %p", p);

    return (void *) p;
}


void
ngx_slab_free(ngx_slab_pool_t *pool, void *p)
{
    ngx_shmtx_lock(&pool->mutex);

    ngx_slab_free_locked(pool, p);

    ngx_shmtx_unlock(&pool->mutex);
}


void
ngx_slab_free_locked(ngx_slab_pool_t *pool, void *p)
{
    size_t            size;
    uintptr_t         slab, m, *bitmap;
    ngx_uint_t        n, type, slot, shift, map;
    ngx_slab_page_t  *slots, *page;

    ngx_log_debug1(NGX_LOG_DEBUG_ALLOC, ngx_cycle->log, 0, "slab free: %p", p);

    if ((u_char *) p < pool->start || (u_char *) p > pool->end) {
        ngx_slab_error(pool, NGX_LOG_ALERT, "ngx_slab_free(): outside of pool");
        goto fail;
    }

    n = ((u_char *) p - pool->start) >> ngx_pagesize_shift;
    page = &pool->pages[n];
    slab = page->slab;
    type = page->prev & NGX_SLAB_PAGE_MASK;

    switch (type) {

    case NGX_SLAB_SMALL:

        shift = slab & NGX_SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        n = ((uintptr_t) p & (ngx_pagesize - 1)) >> shift;
        m = (uintptr_t) 1 << (n & (sizeof(uintptr_t) * 8 - 1));
        n /= (sizeof(uintptr_t) * 8);
        bitmap = (uintptr_t *) ((uintptr_t) p & ~(ngx_pagesize - 1));

        if (bitmap[n] & m) {

            if (page->next == NULL) {
                slots = (ngx_slab_page_t *)
                                   ((u_char *) pool + sizeof(ngx_slab_pool_t));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_SMALL;
                page->next->prev = (uintptr_t) page | NGX_SLAB_SMALL;
            }

            bitmap[n] &= ~m;

            n = (1 << (ngx_pagesize_shift - shift)) / 8 / (1 << shift);

            if (n == 0) {
                n = 1;
            }

            if (bitmap[0] & ~(((uintptr_t) 1 << n) - 1)) {
                goto done;
            }

            map = (1 << (ngx_pagesize_shift - shift)) / (sizeof(uintptr_t) * 8);

            for (n = 1; n < map; n++) {
                if (bitmap[n]) {
                    goto done;
                }
            }

            ngx_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case NGX_SLAB_EXACT:

        m = (uintptr_t) 1 <<
                (((uintptr_t) p & (ngx_pagesize - 1)) >> ngx_slab_exact_shift);
        size = ngx_slab_exact_size;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        if (slab & m) {
            if (slab == NGX_SLAB_BUSY) {
                slots = (ngx_slab_page_t *)
                                   ((u_char *) pool + sizeof(ngx_slab_pool_t));
                slot = ngx_slab_exact_shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_EXACT;
                page->next->prev = (uintptr_t) page | NGX_SLAB_EXACT;
            }

            page->slab &= ~m;

            if (page->slab) {
                goto done;
            }

            ngx_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case NGX_SLAB_BIG:

        shift = slab & NGX_SLAB_SHIFT_MASK;
        size = 1 << shift;

        if ((uintptr_t) p & (size - 1)) {
            goto wrong_chunk;
        }

        m = (uintptr_t) 1 << ((((uintptr_t) p & (ngx_pagesize - 1)) >> shift)
                              + NGX_SLAB_MAP_SHIFT);

        if (slab & m) {

            if (page->next == NULL) {
                slots = (ngx_slab_page_t *)
                                   ((u_char *) pool + sizeof(ngx_slab_pool_t));
                slot = shift - pool->min_shift;

                page->next = slots[slot].next;
                slots[slot].next = page;

                page->prev = (uintptr_t) &slots[slot] | NGX_SLAB_BIG;
                page->next->prev = (uintptr_t) page | NGX_SLAB_BIG;
            }

            page->slab &= ~m;

            if (page->slab & NGX_SLAB_MAP_MASK) {
                goto done;
            }

            ngx_slab_free_pages(pool, page, 1);

            goto done;
        }

        goto chunk_already_free;

    case NGX_SLAB_PAGE:

        if ((uintptr_t) p & (ngx_pagesize - 1)) {
            goto wrong_chunk;
        }

        if (slab == NGX_SLAB_PAGE_FREE) {
            ngx_slab_error(pool, NGX_LOG_ALERT,
                           "ngx_slab_free(): page is already free");
            goto fail;
        }

        if (slab == NGX_SLAB_PAGE_BUSY) {
            ngx_slab_error(pool, NGX_LOG_ALERT,
                           "ngx_slab_free(): pointer to wrong page");
            goto fail;
        }

        n = ((u_char *) p - pool->start) >> ngx_pagesize_shift;
        size = slab & ~NGX_SLAB_PAGE_START;

        ngx_slab_free_pages(pool, &pool->pages[n], size);

        ngx_slab_junk(p, size << ngx_pagesize_shift);

        return;
    }

    /* not reached */

    return;

done:

    ngx_slab_junk(p, size);

    return;

wrong_chunk:

    ngx_slab_error(pool, NGX_LOG_ALERT,
                   "ngx_slab_free(): pointer to wrong chunk");

    goto fail;

chunk_already_free:

    ngx_slab_error(pool, NGX_LOG_ALERT,
                   "ngx_slab_free(): chunk is already free");

fail:

    return;
}


static ngx_slab_page_t *
ngx_slab_alloc_pages(ngx_slab_pool_t *pool, ngx_uint_t pages)
{
    ngx_slab_page_t  *page, *p;

    for (page = pool->free.next; page != &pool->free; page = page->next) {

        if (page->slab >= pages) {	 //page->slab��ʾ���ж����ڴ�ҳ��ʹ��

            if (page->slab > pages) { // ҳ�ڴ��㹻ʹ��
                // �и��pages��ҳ�ڴ����
                page[pages].slab = page->slab - pages;
                page[pages].next = page->next;
                page[pages].prev = page->prev;

                p = (ngx_slab_page_t *) page->prev;
                p->next = &page[pages];
                page->next->prev = (uintptr_t) &page[pages];

            } else {
                p = (ngx_slab_page_t *) page->prev;
                p->next = page->next;
                page->next->prev = page->prev;
            }

            page->slab = pages | NGX_SLAB_PAGE_START;
            page->next = NULL;
            page->prev = NGX_SLAB_PAGE;

            if (--pages == 0) {
                return page;
            }
            // ���µ��ڴ�ҳ��עΪռ��
            for (p = page + 1; pages; pages--) {
                p->slab = NGX_SLAB_PAGE_BUSY;
                p->next = NULL;
                p->prev = NGX_SLAB_PAGE;
                p++;
            }

            return page;
        }
    }

    ngx_slab_error(pool, NGX_LOG_CRIT, "ngx_slab_alloc() failed: no memory");

    return NULL;
}


static void
ngx_slab_free_pages(ngx_slab_pool_t *pool, ngx_slab_page_t *page,
    ngx_uint_t pages)
{
    ngx_slab_page_t  *prev;

    page->slab = pages--;
    // �������pages-1��slab����
    if (pages) {
        ngx_memzero(&page[1], pages * sizeof(ngx_slab_page_t));
    }
    // ��page��ԭ�е�����ɾ��
    if (page->next) {
        prev = (ngx_slab_page_t *) (page->prev & ~NGX_SLAB_PAGE_MASK);
        prev->next = page->next;
        page->next->prev = page->prev;
    }
    // �黹��free����
    page->prev = (uintptr_t) &pool->free;
    page->next = pool->free.next;

    page->next->prev = (uintptr_t) page;

    pool->free.next = page;
}


static void
ngx_slab_error(ngx_slab_pool_t *pool, ngx_uint_t level, char *text)
{
    ngx_log_error(level, ngx_cycle->log, 0, "%s%s", text, pool->log_ctx);
}
