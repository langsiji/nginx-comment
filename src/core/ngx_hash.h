
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _NGX_HASH_H_INCLUDED_
#define _NGX_HASH_H_INCLUDED_


#include <ngx_config.h>
#include <ngx_core.h>

// �����ù�ϣ��֮��Ͳ��ܲ�����ɾ������������õĲ��ǿ�����
// ��ϣ���Ԫ�ش洢���������ڴ������У����ڴ滮�ָ�ÿ��Ͱ��
// ��
// �ڴ�ṹ�ο�http://blog.csdn.net/chen19870707/article/details/40794285


// �����ϣ���Ԫ��
typedef struct {
    void             *value;
    u_short           len; // name����
    u_char            name[1];
} ngx_hash_elt_t;


typedef struct {
    ngx_hash_elt_t  **buckets; // Ͱ����
    ngx_uint_t        size; // Ͱ����
} ngx_hash_t;


typedef struct {
    ngx_hash_t        hash;
    void             *value;
} ngx_hash_wildcard_t;

// ��ϣ��Ԫ�ؼ�/ֵ�ԣ���ʼ����ʱ��ʹ��
typedef struct {
    ngx_str_t         key;
    ngx_uint_t        key_hash; // ͨ��key����õĹ�ϣֵ��ͨ������ķ�ʽ֪��������ĸ�Ͱ���й�
    void             *value;
} ngx_hash_key_t;


typedef ngx_uint_t (*ngx_hash_key_pt) (u_char *data, size_t len);


typedef struct {
    ngx_hash_t            hash;
    ngx_hash_wildcard_t  *wc_head;
    ngx_hash_wildcard_t  *wc_tail;
} ngx_hash_combined_t;
/*
hash:	���ֶ����ΪNULL����ô�������ʼ��������
���ֶ�ָ���´���������hash��������ֶβ�ΪNULL��
��ô�ڳ�ʼ��ʱ�����е����ݱ�����������ֶ���ָ��hash���С�

key:	ָ����ַ�������hashֵ��hash������
nginx��Դ�������ṩ��Ĭ�ϵ�ʵ�ֺ���ngx_hash_key_lc��

max_size:	hash���е�Ͱ�ĸ��������ֶ�Խ��
Ԫ�ش洢ʱ��ͻ�Ŀ�����ԽС��ÿ��Ͱ�д洢��Ԫ�ػ���٣�
���ѯ�������ٶȸ��졣��Ȼ�����ֵԽ��Խ����ڴ����
��ҲԽ��(ʵ����Ҳ�˷Ѳ��˶���)��

bucket_size:	ÿ��Ͱ��������ƴ�С����λ���ֽڡ�
����ڳ�ʼ��һ��hash���ʱ�򣬷���ĳ��Ͱ��
���޷�������������ڸ�Ͱ��Ԫ�أ���hash���ʼ��ʧ�ܡ�

name:	��hash������֡�

pool:	��hash������ڴ�ʹ�õ�pool��

temp_pool:	��hash��ʹ�õ���ʱpool���ڳ�ʼ������Ժ�
��pool���Ա��ͷź����ٵ�
*/

typedef struct {
	// ��ϣͰ�ṹ
    ngx_hash_t       *hash;
	// hash����ָ��
    ngx_hash_key_pt   key;
	// Ͱ������
    ngx_uint_t        max_size;
	// ÿ��ͰԪ�ش�С
    ngx_uint_t        bucket_size;
	// ��ϣ������ƣ�������־���
    char             *name;
	// �����ڴ��
    ngx_pool_t       *pool;
	// ��ʱ�ڴ�أ����ڳ�ʼ��
    ngx_pool_t       *temp_pool;
} ngx_hash_init_t;


#define NGX_HASH_SMALL            1
#define NGX_HASH_LARGE            2

#define NGX_HASH_LARGE_ASIZE      16384
#define NGX_HASH_LARGE_HSIZE      10007

#define NGX_HASH_WILDCARD_KEY     1
#define NGX_HASH_READONLY_KEY     2


typedef struct {
    ngx_uint_t        hsize;

    ngx_pool_t       *pool;
    ngx_pool_t       *temp_pool;

    ngx_array_t       keys;
    ngx_array_t      *keys_hash;

    ngx_array_t       dns_wc_head;
    ngx_array_t      *dns_wc_head_hash;

    ngx_array_t       dns_wc_tail;
    ngx_array_t      *dns_wc_tail_hash;
} ngx_hash_keys_arrays_t;


typedef struct {
    ngx_uint_t        hash;
    ngx_str_t         key;
    ngx_str_t         value;
    u_char           *lowcase_key;
} ngx_table_elt_t;


void *ngx_hash_find(ngx_hash_t *hash, ngx_uint_t key, u_char *name, size_t len);
void *ngx_hash_find_wc_head(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_wc_tail(ngx_hash_wildcard_t *hwc, u_char *name, size_t len);
void *ngx_hash_find_combined(ngx_hash_combined_t *hash, ngx_uint_t key,
    u_char *name, size_t len);

ngx_int_t ngx_hash_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);
ngx_int_t ngx_hash_wildcard_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts);

#define ngx_hash(key, c)   ((ngx_uint_t) key * 31 + c)
ngx_uint_t ngx_hash_key(u_char *data, size_t len);
ngx_uint_t ngx_hash_key_lc(u_char *data, size_t len);
ngx_uint_t ngx_hash_strlow(u_char *dst, u_char *src, size_t n);


ngx_int_t ngx_hash_keys_array_init(ngx_hash_keys_arrays_t *ha, ngx_uint_t type);
ngx_int_t ngx_hash_add_key(ngx_hash_keys_arrays_t *ha, ngx_str_t *key,
    void *value, ngx_uint_t flags);


#endif /* _NGX_HASH_H_INCLUDED_ */
