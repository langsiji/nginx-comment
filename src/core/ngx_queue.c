
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


/*
 * find the middle queue element if the queue has odd number of elements
 * or the first element of the queue's second part otherwise
 */
// �ҵ��м�ڵ㣬ʹ�õķ�����������ָ��middle��next
// ͬʱ��ʼ�ƶ���ÿ���ƶ�middle�ƶ�һ����next�ƶ�����
// ��next�����middle��Ϊ�м�ڵ�
// ������нڵ���Ϊ�����������м䣬Ϊż�����صڶ�
// ���ֵĵ�һ��Ԫ��
// ��:
// 		[1,2,3,4,5]����3
//		[1,2,3,4]����3
ngx_queue_t *
ngx_queue_middle(ngx_queue_t *queue)
{
    ngx_queue_t  *middle, *next;
	// ֻ��һ���ڵ����Ϊ��
    middle = ngx_queue_head(queue);

    if (middle == ngx_queue_last(queue)) {
        return middle;
    }

    next = ngx_queue_head(queue);

    for ( ;; ) {
        middle = ngx_queue_next(middle);

        next = ngx_queue_next(next);

        if (next == ngx_queue_last(queue)) {
            return middle;
        }

        next = ngx_queue_next(next);

        if (next == ngx_queue_last(queue)) {
            return middle;
        }
    }
}


/* the stable insertion sort */

void
ngx_queue_sort(ngx_queue_t *queue,
    ngx_int_t (*cmp)(const ngx_queue_t *, const ngx_queue_t *))
{
    ngx_queue_t  *q, *prev, *next;

	// ֻ��һ���ڵ����Ϊ��
    q = ngx_queue_head(queue);

    if (q == ngx_queue_last(queue)) {
        return;
    }

    for (q = ngx_queue_next(q); q != ngx_queue_sentinel(queue); q = next) {

        prev = ngx_queue_prev(q);
        next = ngx_queue_next(q);

        ngx_queue_remove(q);

        do {
            if (cmp(prev, q) <= 0) {
                break;
            }

            prev = ngx_queue_prev(prev);

        } while (prev != ngx_queue_sentinel(queue));
		// q > prev����嵽prev����
        ngx_queue_insert_after(prev, q);
    }
}
