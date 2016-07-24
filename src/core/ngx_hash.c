
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <ngx_config.h>
#include <ngx_core.h>


void *
ngx_hash_find(ngx_hash_t *hash, ngx_uint_t key, u_char *name, size_t len)
{
    ngx_uint_t       i;
    ngx_hash_elt_t  *elt;

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "hf:\"%*s\"", len, name);
#endif

    elt = hash->buckets[key % hash->size];

    if (elt == NULL) {
        return NULL;
    }

    while (elt->value) { // ����β����value��NULLָ�룬value�ڽṹ��ĵ�һ���ֶ�
        if (len != (size_t) elt->len) {
            goto next;
        }
		// �Ƚ�key
        for (i = 0; i < len; i++) {
            if (name[i] != elt->name[i]) {
                goto next;
            }
        }

        return elt->value;

    next:
		// �������飬ָ����һ��Ԫ��
        elt = (ngx_hash_elt_t *) ngx_align_ptr(&elt->name[0] + elt->len,
                                               sizeof(void *));
        continue;
    }

    return NULL;
}

// �Ҳ�������hwc->value
void *
ngx_hash_find_wc_head(ngx_hash_wildcard_t *hwc, u_char *name, size_t len)
{
    void        *value;
    ngx_uint_t   i, n, key;

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "wch:\"%*s\"", len, name);
#endif
	// ��β����ʼ��ǰ�ң���n �� len-1 ��Ϊ�ؼ��������һ�� �ӹؼ���
    n = len;

    while (n) {
        if (name[n - 1] == '.') {
            break;
        }

        n--;
    }

    key = 0;

	// ����n��len-1�ַ���hashֵ
    for (i = n; i < len; i++) {
        key = ngx_hash(key, name[i]);
    }


	//������ͨ�����ҵ��ؼ��ֵ�value���û��Զ�������ָ�룩

    value = ngx_hash_find(&hwc->hash, key, &name[n], len - n);

/**
* ��valueָ���2λ��Я����Ϣ�����£�

* 00 - value �� "example.com" �� "*.example.com"������ָ��

* 01 - value ������ "*.example.com"������ָ��

* 10 - value �� ֧��ͨ�����ϣ���� "example.com" �� "*.example.com" ָ��

* 11 - value ������ "*.example.com"��ָ��

*/


    if (value) {

        /* ע��ָ�������ָ�룬data pointer ��pointer������
         * the 2 low bits of value have the special meaning:
         *     00 - value is data pointer for both "example.com"
         *          and "*.example.com";
         *     01 - value is data pointer for "*.example.com" only;
         *     10 - value is pointer to wildcard hash allowing
         *          both "example.com" and "*.example.com";
         *     11 - value is pointer to wildcard hash allowing
         *          "*.example.com" only.
         */

        if ((uintptr_t) value & 2) {

            if (n == 0) { // n==0��ȫƥ��β׺

                /* "example.com" */

                if ((uintptr_t) value & 1) {
                    return NULL;
                }

                hwc = (ngx_hash_wildcard_t *)
                                          ((uintptr_t) value & (uintptr_t) ~3);
                return hwc->value;
            }

            hwc = (ngx_hash_wildcard_t *) ((uintptr_t) value & (uintptr_t) ~3);

            value = ngx_hash_find_wc_head(hwc, name, n - 1); // -1ȥ��.���ţ��ݹ����²���

            if (value) {
                return value;
            }

            return hwc->value;
        }

        if ((uintptr_t) value & 1) {

            if (n == 0) {

                /* "example.com" */

                return NULL;
            }

            return (void *) ((uintptr_t) value & (uintptr_t) ~3);
        }

        return value;
    }
    return hwc->value;
}


void *
ngx_hash_find_wc_tail(ngx_hash_wildcard_t *hwc, u_char *name, size_t len)
{
    void        *value;
    ngx_uint_t   i, key;

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "wct:\"%*s\"", len, name);
#endif

    key = 0;

    for (i = 0; i < len; i++) {
        if (name[i] == '.') {
            break;
        }

        key = ngx_hash(key, name[i]);
    }

    if (i == len) {
        return NULL;
    }

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "key:\"%ui\"", key);
#endif

    value = ngx_hash_find(&hwc->hash, key, name, i);

#if 0
    ngx_log_error(NGX_LOG_ALERT, ngx_cycle->log, 0, "value:\"%p\"", value);
#endif

    if (value) {

        /*
         * the 2 low bits of value have the special meaning:
         *     00 - value is data pointer;
         *     11 - value is pointer to wildcard hash allowing "example.*".
         */

        if ((uintptr_t) value & 2) {

            i++;

            hwc = (ngx_hash_wildcard_t *) ((uintptr_t) value & (uintptr_t) ~3);

            value = ngx_hash_find_wc_tail(hwc, &name[i], len - i);

            if (value) {
                return value;
            }

            return hwc->value;
        }

        return value;
    }

    return hwc->value;
}

// ������ͨ��ϣ���в��ң�û�ҵ���ȥǰ��ͨ�����ϣ���в��ң�
// ���ȥ����ͨ�����ϣ���в���

void *
ngx_hash_find_combined(ngx_hash_combined_t *hash, ngx_uint_t key, u_char *name,
    size_t len)
{
    void  *value;

    if (hash->hash.buckets) {
        value = ngx_hash_find(&hash->hash, key, name, len);

        if (value) {
            return value;
        }
    }

    if (len == 0) {
        return NULL;
    }

    if (hash->wc_head && hash->wc_head->hash.buckets) {
        value = ngx_hash_find_wc_head(hash->wc_head, name, len);

        if (value) {
            return value;
        }
    }

    if (hash->wc_tail && hash->wc_tail->hash.buckets) {
        value = ngx_hash_find_wc_tail(hash->wc_tail, name, len);

        if (value) {
            return value;
        }
    }

    return NULL;
}


// ����ngx_hash_elt_tռ���ڴ��С

// ע���ϣԪ�صļ��ֽڶ���ngx_align((name)->key.len + 2, sizeof(void *))
// ����һ��ָ���һ����
// sizeof(void*)�洢value��2�洢len��key.len�洢key
#define NGX_HASH_ELT_SIZE(name)                                               \
    (sizeof(void *) + ngx_align((name)->key.len + 2, sizeof(void *)))

ngx_int_t
ngx_hash_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names, ngx_uint_t nelts)
{
    u_char          *elts;
    size_t           len;
    u_short         *test; // �������ڲ�ͬλ�ñ�ʾ��ͬ����
    ngx_uint_t       i, n, key, size, start, bucket_size;
    ngx_hash_elt_t  *elt, **buckets;
	//һ��Ͱ���Դ洢���Ԫ�أ�Ͱʹ�õĲ��ǿ�����
    for (n = 0; n < nelts; n++) {
		// ÿ��Ԫ��ռ�ÿռ�С��ͰԪ�ؿռ��С
        if (hinit->bucket_size < NGX_HASH_ELT_SIZE(&names[n]) + sizeof(void *))
        {
            ngx_log_error(NGX_LOG_EMERG, hinit->pool->log, 0,
                          "could not build the %s, you should "
                          "increase %s_bucket_size: %i",
                          hinit->name, hinit->name, hinit->bucket_size);
            return NGX_ERROR;
        }
    }

	// �ҵ����ʵ�Ͱ����
    test = ngx_alloc(hinit->max_size * sizeof(u_short), hinit->pool->log);
    if (test == NULL) {
        return NGX_ERROR;
    }

    bucket_size = hinit->bucket_size - sizeof(void *); // ÿ��Ͱ��Ҫ����һ��void*ָ�룬�����ռ䱣��Ԫ��

	// �����ʼͰ�Ĵ�С����̽Ͱ�������Ƿ����

	/* ������ҪͰ��Ŀ���½�

	ÿ��Ԫ��������Ҫ NGX_HASH_ELT_SIZE(&name[n]) > (2*sizeof(void*)) �Ŀռ�

	��� bucket_size ��С��Ͱ��������� bucket_size/(2*sizeof(void*)) ��Ԫ��

	��� nelts ��Ԫ�ؾ�������Ҫstart��Ͱ��

	*/
    start = nelts / (bucket_size / (2 * sizeof(void *)));
    start = start ? start : 1;

    if (hinit->max_size > 10000 && nelts && hinit->max_size / nelts < 100) {
        start = hinit->max_size - 1000;
    }

	// ����СͰ��ʼ�������ϣ��������Ҫ���ٸ�Ͱ
    for (size = start; size < hinit->max_size; size++) {

        ngx_memzero(test, size * sizeof(u_short));

        for (n = 0; n < nelts; n++) {
            if (names[n].key.data == NULL) {
                continue;
            }

            key = names[n].key_hash % size;
			// ������ײtest[key]��С��0��˵�����ͰҪ�洢���Ԫ��
            test[key] = (u_short) (test[key] + NGX_HASH_ELT_SIZE(&names[n]));

#if 0
            ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: %ui %ui \"%V\"",
                          size, key, test[key], &names[n].key);
#endif
			// ��֤ÿ��ͰС�ڹ涨��С�����������С����ô
			// ����Ͱ������������ײ
            if (test[key] > (u_short) bucket_size) { // ����Ͱ����+1���ټ�����ʵ�Ͱ�����С
                goto next;
            }
        }
		// �Ѿ��ҵ����ʵ�Ͱ����
        goto found;

    next:

        continue;
    }

    ngx_log_error(NGX_LOG_EMERG, hinit->pool->log, 0,
                  "could not build the %s, you should increase "
                  "either %s_max_size: %i or %s_bucket_size: %i",
                  hinit->name, hinit->name, hinit->max_size,
                  hinit->name, hinit->bucket_size);

    ngx_free(test);

    return NGX_ERROR;

found:
	// sizeΪͰ����

    for (i = 0; i < size; i++) {
        test[i] = sizeof(void *); // ����β��NULLָ��
    }
	//����testÿ��Ͱռ���ڴ�Ĵ�С
    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }
		// �������㷨һ��
        key = names[n].key_hash % size;
        test[key] = (u_short) (test[key] + NGX_HASH_ELT_SIZE(&names[n]));
    }

    len = 0; // �ܵ��ڴ��С

    for (i = 0; i < size; i++) {
        if (test[i] == sizeof(void *)) {
			// ��Ͱû��Ԫ��
            continue;
        }
		// ���ϵ��������ֽ�
        test[i] = (u_short) (ngx_align(test[i], ngx_cacheline_size));

        len += test[i];
    }
	// ngx_hash_wildcard_t ����
    if (hinit->hash == NULL) {
        hinit->hash = ngx_pcalloc(hinit->pool, sizeof(ngx_hash_wildcard_t)
                                             + size * sizeof(ngx_hash_elt_t *));
        if (hinit->hash == NULL) {
            ngx_free(test);
            return NGX_ERROR;
        }

        buckets = (ngx_hash_elt_t **)
                      ((u_char *) hinit->hash + sizeof(ngx_hash_wildcard_t));

    } else {
        buckets = ngx_pcalloc(hinit->pool, size * sizeof(ngx_hash_elt_t *));
        if (buckets == NULL) {
            ngx_free(test);
            return NGX_ERROR;
        }
    }

    elts = ngx_palloc(hinit->pool, len + ngx_cacheline_size);
    if (elts == NULL) {
        ngx_free(test);
        return NGX_ERROR;
    }

    elts = ngx_align_ptr(elts, ngx_cacheline_size);
	// ��ÿ��Ͱָ���Ӧ���ڴ���������
	// ���֣���ϣ���е�Ԫ������һ������
	// �ڴ��У�ֻ�ǽ���������ڴ����
	// Ͱ�����֣�ĳ���ڴ�����ĳ��Ͱ��
	// ���ڴ��С��ÿ��Ͱ�����Ԫ�ض���
	// ������
    for (i = 0; i < size; i++) {
		// ˵����Ͱû��Ԫ��
        if (test[i] == sizeof(void *)) {
            continue;
        }

        buckets[i] = (ngx_hash_elt_t *) elts;
        elts += test[i]; // ƫ�Ƶ���һ��Ͱ����ʼλ�ã�test[i]Ԥ���Ѿ������

    }

    for (i = 0; i < size; i++) {
        test[i] = 0;
    }

	// ��Ԫ�ؿ�������ϣ����
	// test[i]��ʾĳ��Ͱ�Ѿ�ʹ�õ��ڴ��С��
	// �����뿪�����е�Ͱͷָ��
    for (n = 0; n < nelts; n++) {
        if (names[n].key.data == NULL) {
            continue;
        }

        key = names[n].key_hash % size;
        elt = (ngx_hash_elt_t *) ((u_char *) buckets[key] + test[key]);

        elt->value = names[n].value;
        elt->len = (u_short) names[n].key.len;

        ngx_strlow(elt->name, names[n].key.data, names[n].key.len);
		// ��Ͱ��һ��Ԫ�ش洢����ʼָ��
        test[key] = (u_short) (test[key] + NGX_HASH_ELT_SIZE(&names[n]));
    }

    for (i = 0; i < size; i++) {
		// ��Ͱû��Ԫ��
        if (buckets[i] == NULL) {
            continue;
        }
		// β����NULL���ж��������
        elt = (ngx_hash_elt_t *) ((u_char *) buckets[i] + test[i]);

        elt->value = NULL;
    }

    ngx_free(test);

    hinit->hash->buckets = buckets;
    hinit->hash->size = size;

#if 0

    for (i = 0; i < size; i++) {
        ngx_str_t   val;
        ngx_uint_t  key;

        elt = buckets[i];

        if (elt == NULL) {
            ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: NULL", i);
            continue;
        }

        while (elt->value) {
            val.len = elt->len;
            val.data = &elt->name[0];

            key = hinit->key(val.data, val.len);

            ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                          "%ui: %p \"%V\" %ui", i, elt, &val, key);

            elt = (ngx_hash_elt_t *) ngx_align_ptr(&elt->name[0] + elt->len,
                                                   sizeof(void *));
        }
    }

#endif

    return NGX_OK;
}


/*hinitΪ��ʼ���ṹ��ָ�룬namesΪԤ�����ϣ�����飬eltsΪԤ���������С

�ر�Ҫע����������key�Ѿ����Ǳ�Ԥ������ġ����磺��*.abc.com�����ߡ�.abc.com����Ԥ��������Ժ�

����ˡ�com.abc.��������mail.xxx.*����Ԥ����Ϊ��mail.xxx.��
*/
/*
nginxΪ�˴������ͨ�����������ƥ�����⣬ʵ����ngx_hash_wildcard_t������hash��
������֧���������͵Ĵ���ͨ�����������һ����ͨ�����ǰ�ģ�
���磺��*.abc.com����Ҳ����ʡ�Ե��Ǻţ�ֱ��д�ɡ�.abc.com����������key��
����ƥ��www.abc.com��qqq.www.abc.com֮��ġ�����һ����ͨ�����ĩβ�ģ�
���磺��mail.xxx.*�������ر�ע��ͨ�����ĩβ�Ĳ���λ�ڿ�ʼ��ͨ���
���Ա�ʡ�Ե���������ͨ���������ƥ��mail.xxx.com��mail.xxx.com.cn��mail.xxx.net֮
���������

��һ�����˵��������һ��ngx_hash_wildcard_t���͵�hash��ֻ�ܰ���ͨ�����ǰ
��key������ͨ����ں��key������ͬʱ�����������͵�ͨ�����key��
ngx_hash_wildcard_t���ͱ����Ĺ�����ͨ������ngx_hash_wildcard_init��ɵģ�
����ѯ��ͨ������ngx_hash_find_wc_head����ngx_hash_find_wc_tail�����ġ�
ngx_hash_find_wc_head�ǲ�ѯ����ͨ�����ǰ��key��hash��ģ�
��ngx_hash_find_wc_tail�ǲ�ѯ����ͨ����ں��key��hash���


�����hash������е�ͨ���key�����顣
�ر�Ҫע����������key�Ѿ����Ǳ�Ԥ������ġ�
���磺��*.abc.com�����ߡ�.abc.com����Ԥ��������Ժ�
����ˡ�com.abc.��������mail.xxx.*����Ԥ����Ϊ��mail.xxx.����
Ϊʲô�ᱻ�������������ﲻ�ò��򵥵�����һ��ͨ���hash���ʵ��ԭ��
����������͵�hash���ʱ��ʵ�����ǹ�����һ��hash���һ����������
��ͨ��hash���е�key�����ӡ������ġ����磺���ڡ�*.abc.com�����ṹ���2��hash��
��һ��hash������һ��keyΪcom�ı���ñ����value������ָ��ڶ���hash���ָ�룬
���ڶ���hash������һ������abc���ñ����value������ָ��*.abc.com��Ӧ��value��ָ�롣
��ô��ѯ��ʱ�򣬱����ѯwww.abc.com��ʱ���Ȳ�com��ͨ����com�����ҵ��ڶ�����
hash���ڵڶ���hash���У��ٲ���abc���������ƣ�ֱ����ĳһ����hash���в鵽��
�����Ӧ��value��Ӧһ��������ֵ����һ��ָ����һ��hash���ָ���ʱ��
��ѯ���̽�����������һ����Ҫ�ر�ע��ģ�����names������Ԫ�ص�valueֵ��
��λbit����Ϊ0����������;�������������������������hash���ѯ������
ȷ�����

����ָ�붼�ֽڶ����ˣ���4λ�϶�Ϊ0��
���ֲ�����name->value = (void *) ((uintptr_t) wdc | (dot ? 3 : 2)) �� ,
ʹ����ָ��ĵ�λЯ��������Ϣ����ʡ���ڴ�

*/
ngx_int_t
ngx_hash_wildcard_init(ngx_hash_init_t *hinit, ngx_hash_key_t *names,
    ngx_uint_t nelts)
{
    size_t                len, dot_len;
    ngx_uint_t            i, n, dot;
    ngx_array_t           curr_names, next_names;
    ngx_hash_key_t       *name, *next_name;
    ngx_hash_init_t       h;
    ngx_hash_wildcard_t  *wdc;

    if (ngx_array_init(&curr_names, hinit->temp_pool, nelts,
                       sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (ngx_array_init(&next_names, hinit->temp_pool, nelts,
                       sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    for (n = 0; n < nelts; n = i) {

#if 0
        ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                      "wc0: \"%V\"", &names[n].key);
#endif

        dot = 0;
		//www.example.com ��Ԥ����Ϊcom.example.www
        for (len = 0; len < names[n].key.len; len++) {
            if (names[n].key.data[len] == '.') {
                dot = 1;
                break;
            }
        }
		//���ؼ���dot��ǰ�Ĺؼ��ַ���curr_names������www.example.com����nameΪcom
        name = ngx_array_push(&curr_names);
        if (name == NULL) {
            return NGX_ERROR;
        }

        name->key.len = len;
        name->key.data = names[n].key.data;
        name->key_hash = hinit->key(name->key.data, name->key.len); // ����hashֵ
        name->value = names[n].value;

		
        dot_len = len + 1;
		//lenָ��dot��ʣ��ؼ���
        if (dot) {
            len++;
        }

        next_names.nelts = 0;
		
		//���names[n] dot����ʣ��ؼ��֣���ʣ��ؼ��ַ���next_names��
        if (names[n].key.len != len) {
            next_name = ngx_array_push(&next_names);
            if (next_name == NULL) {
                return NGX_ERROR;
            }
			//www.example
            next_name->key.len = names[n].key.len - len; // dot�����ַ�����
            next_name->key.data = names[n].key.data + len; // ָ��dot���������
            next_name->key_hash = 0;
            next_name->value = names[n].value;

#if 0
            ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                          "wc2: \"%V\"", &next_name->key);
#endif
        }
		//��n+1����names��������ͬβ׺���ַ�����next_names����
        for (i = n + 1; i < nelts; i++) {
			// ����ͬβ׺
            if (ngx_strncmp(names[n].key.data, names[i].key.data, len) != 0) {
                break;
            }

            if (!dot
                && names[i].key.len > len
                && names[i].key.data[len] != '.')
            {
                break;
            }

            next_name = ngx_array_push(&next_names);
            if (next_name == NULL) {
                return NGX_ERROR;
            }

			// ���µ��ַ�����next_names
            next_name->key.len = names[i].key.len - dot_len;
            next_name->key.data = names[i].key.data + dot_len;
            next_name->key_hash = 0;
            next_name->value = names[i].value;

#if 0
            ngx_log_error(NGX_LOG_ALERT, hinit->pool->log, 0,
                          "wc3: \"%V\"", &next_name->key);
#endif
        }
		// ���������ϣcom.abc.www���������洦��next_names�������
		// abc.www��
        if (next_names.nelts) {

            h = *hinit;
            h.hash = NULL;
			// �ݹ鴴���µĹ�ϣ��
            if (ngx_hash_wildcard_init(&h, (ngx_hash_key_t *) next_names.elts,
                                       next_names.nelts)
                != NGX_OK)
            {
                return NGX_ERROR;
            }

            wdc = (ngx_hash_wildcard_t *) h.hash;
			// ���û�valueֵ�����µ�hash��
            if (names[n].key.len == len) {
                wdc->value = names[n].value;
            }

            name->value = (void *) ((uintptr_t) wdc | (dot ? 3 : 2)); // ��curr_names��

        } else if (dot) {
            name->value = (void *) ((uintptr_t) name->value | 1);
        }
    }
	// �Ѿ������ö���������...��ϣ��֮�󣬹����ײ��ϣ��
    if (ngx_hash_init(hinit, (ngx_hash_key_t *) curr_names.elts,
                      curr_names.nelts)
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    return NGX_OK;
}

// ��ϣ����
ngx_uint_t
ngx_hash_key(u_char *data, size_t len)
{
    ngx_uint_t  i, key;

    key = 0;

    for (i = 0; i < len; i++) {
        key = ngx_hash(key, data[i]);
    }

    return key;
}

// �ַ�תСд���ϣ
ngx_uint_t
ngx_hash_key_lc(u_char *data, size_t len)
{
    ngx_uint_t  i, key;

    key = 0;

    for (i = 0; i < len; i++) {
        key = ngx_hash(key, ngx_tolower(data[i]));
    }

    return key;
}

// strlow
ngx_uint_t
ngx_hash_strlow(u_char *dst, u_char *src, size_t n)
{
    ngx_uint_t  key;

    key = 0;

    while (n--) {
        *dst = ngx_tolower(*src);
        key = ngx_hash(key, *dst);
        dst++;
        src++;
    }

    return key;
}

/*

hsize:	��Ҫ������hash���Ͱ�ĸ���������ʹ������ṹ�а�������Ϣ�������������͵�hash����ʹ�ô˲�����
pool:	������Щhash��ʹ�õ�pool��
temp_pool:	�ڹ�����������Լ����յ�����hash������п����õ���ʱpool����temp_pool�����ڹ�������Ժ󣬱����ٵ�������ֻ�Ǵ����ʱ��һЩ�ڴ����ġ�
keys:	������з�ͨ���key�����顣
keys_hash:	���Ǹ���ά���飬��һ��ά�ȴ������bucket�ı�ţ���ôkeys_hash[i]�д�ŵ������е�key�������hashֵ��hsizeȡģ�Ժ��ֵΪi��key��������3��key,�ֱ���key1,key2��key3����hashֵ������Ժ��hsizeȡģ��ֵ����i����ô������key��ֵ��˳������keys_hash[i][0],keys_hash[i][1], keys_hash[i][2]����ֵ�ڵ��õĹ�������������ͼ���Ƿ��г�ͻ��keyֵ��Ҳ�����Ƿ����ظ���
dns_wc_head:	��ǰ��ͨ���key����������Ժ��ֵ�����磺��*.abc.com�� ����������Ժ󣬱�� ��com.abc.�� ������ڴ������С�
dns_wc_tail:	��ź���ͨ���key����������Ժ��ֵ�����磺��mail.xxx.*�� ����������Ժ󣬱�� ��mail.xxx.�� ������ڴ������С�
dns_wc_head_hash:
 	��ֵ�ڵ��õĹ�������������ͼ���Ƿ��г�ͻ��ǰ��ͨ�����keyֵ��Ҳ�����Ƿ����ظ���
dns_wc_tail_hash:
 	��ֵ�ڵ��õĹ�������������ͼ���Ƿ��г�ͻ�ĺ���ͨ�����keyֵ��Ҳ�����Ƿ����ظ���

*/
ngx_int_t
ngx_hash_keys_array_init(ngx_hash_keys_arrays_t *ha, ngx_uint_t type)
{
    ngx_uint_t  asize;

    if (type == NGX_HASH_SMALL) {
        asize = 4;
        ha->hsize = 107;

    } else {
        asize = NGX_HASH_LARGE_ASIZE;
        ha->hsize = NGX_HASH_LARGE_HSIZE;
    }

    if (ngx_array_init(&ha->keys, ha->temp_pool, asize, sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (ngx_array_init(&ha->dns_wc_head, ha->temp_pool, asize,
                       sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    if (ngx_array_init(&ha->dns_wc_tail, ha->temp_pool, asize,
                       sizeof(ngx_hash_key_t))
        != NGX_OK)
    {
        return NGX_ERROR;
    }

    ha->keys_hash = ngx_pcalloc(ha->temp_pool, sizeof(ngx_array_t) * ha->hsize);
    if (ha->keys_hash == NULL) {
        return NGX_ERROR;
    }

    ha->dns_wc_head_hash = ngx_pcalloc(ha->temp_pool,
                                       sizeof(ngx_array_t) * ha->hsize);
    if (ha->dns_wc_head_hash == NULL) {
        return NGX_ERROR;
    }

    ha->dns_wc_tail_hash = ngx_pcalloc(ha->temp_pool,
                                       sizeof(ngx_array_t) * ha->hsize);
    if (ha->dns_wc_tail_hash == NULL) {
        return NGX_ERROR;
    }

    return NGX_OK;
}


ngx_int_t
ngx_hash_add_key(ngx_hash_keys_arrays_t *ha, ngx_str_t *key, void *value,
    ngx_uint_t flags)
{
    size_t           len;
    u_char          *p;
    ngx_str_t       *name;
    ngx_uint_t       i, k, n, skip, last;
    ngx_array_t     *keys, *hwc;
    ngx_hash_key_t  *hk;

    last = key->len;

    if (flags & NGX_HASH_WILDCARD_KEY) {

        /*
         * supported wildcards:
         *     "*.example.com", ".example.com", and "www.example.*"
         */

        n = 0;
		// �����ȷ��
        for (i = 0; i < key->len; i++) {

            if (key->data[i] == '*') {
                if (++n > 1) {
                    return NGX_DECLINED;
                }
            }

            if (key->data[i] == '.' && key->data[i + 1] == '.') {
                return NGX_DECLINED;
            }
        }
		//.example.com
        if (key->len > 1 && key->data[0] == '.') {
            skip = 1;
            goto wildcard;
        }

        if (key->len > 2) {
			//*.example.com
            if (key->data[0] == '*' && key->data[1] == '.') {
                skip = 2;
                goto wildcard;
            }
			//www.example.*
            if (key->data[i - 2] == '.' && key->data[i - 1] == '*') {
                skip = 0;
                last -= 2;
                goto wildcard;
            }
        }
		// ������ͳ������м�
        if (n) {
            return NGX_DECLINED;
        }
    }

    /* exact hash */
	// û��ͨ�����Ԫ��
    k = 0;

    for (i = 0; i < last; i++) {
        if (!(flags & NGX_HASH_READONLY_KEY)) {
            key->data[i] = ngx_tolower(key->data[i]);
        }
        k = ngx_hash(k, key->data[i]);
    }

    k %= ha->hsize;

    /* check conflicts in exact hash */

    name = ha->keys_hash[k].elts;

    if (name) {
        for (i = 0; i < ha->keys_hash[k].nelts; i++) {
            if (last != name[i].len) {
                continue;
            }
			// �Ѿ�����
            if (ngx_strncmp(key->data, name[i].data, last) == 0) {
                return NGX_BUSY;
            }
        }

    } else {
        if (ngx_array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                           sizeof(ngx_str_t))
            != NGX_OK)
        {
            return NGX_ERROR;
        }
    }
	// �����Ӧ��Ͱ
    name = ngx_array_push(&ha->keys_hash[k]);
    if (name == NULL) {
        return NGX_ERROR;
    }

    *name = *key;

    hk = ngx_array_push(&ha->keys);
    if (hk == NULL) {
        return NGX_ERROR;
    }

    hk->key = *key;
    hk->key_hash = ngx_hash_key(key->data, last);
    hk->value = value;

    return NGX_OK;


wildcard:

    /* wildcard hash */

    k = ngx_hash_strlow(&key->data[skip], &key->data[skip], last - skip);

    k %= ha->hsize;

    if (skip == 1) {

        /* check conflicts in exact hash for ".example.com" */

        name = ha->keys_hash[k].elts;

        if (name) {
            len = last - skip;

            for (i = 0; i < ha->keys_hash[k].nelts; i++) {
                if (len != name[i].len) {
                    continue;
                }

                if (ngx_strncmp(&key->data[1], name[i].data, len) == 0) {
                    return NGX_BUSY;
                }
            }

        } else {
            if (ngx_array_init(&ha->keys_hash[k], ha->temp_pool, 4,
                               sizeof(ngx_str_t))
                != NGX_OK)
            {
                return NGX_ERROR;
            }
        }

        name = ngx_array_push(&ha->keys_hash[k]);
        if (name == NULL) {
            return NGX_ERROR;
        }

        name->len = last - 1;
        name->data = ngx_pnalloc(ha->temp_pool, name->len);
        if (name->data == NULL) {
            return NGX_ERROR;
        }

        ngx_memcpy(name->data, &key->data[1], name->len);
    }


    if (skip) {

        /*
         * convert "*.example.com" to "com.example.\0"
         *      and ".example.com" to "com.example\0"
         */

        p = ngx_pnalloc(ha->temp_pool, last);
        if (p == NULL) {
            return NGX_ERROR;
        }

        len = 0;
        n = 0;

        for (i = last - 1; i; i--) {
            if (key->data[i] == '.') {
                ngx_memcpy(&p[n], &key->data[i + 1], len);
                n += len;
                p[n++] = '.';
                len = 0;
                continue;
            }

            len++;
        }

        if (len) {
            ngx_memcpy(&p[n], &key->data[1], len);
            n += len;
        }

        p[n] = '\0';

        hwc = &ha->dns_wc_head;
        keys = &ha->dns_wc_head_hash[k];

    } else {

        /* convert "www.example.*" to "www.example\0" */

        last++;

        p = ngx_pnalloc(ha->temp_pool, last);
        if (p == NULL) {
            return NGX_ERROR;
        }

        ngx_cpystrn(p, key->data, last);

        hwc = &ha->dns_wc_tail;
        keys = &ha->dns_wc_tail_hash[k];
    }


    /* check conflicts in wildcard hash */

    name = keys->elts;

    if (name) {
        len = last - skip;

        for (i = 0; i < keys->nelts; i++) {
            if (len != name[i].len) {
                continue;
            }

            if (ngx_strncmp(key->data + skip, name[i].data, len) == 0) {
                return NGX_BUSY;
            }
        }

    } else {
        if (ngx_array_init(keys, ha->temp_pool, 4, sizeof(ngx_str_t)) != NGX_OK)
        {
            return NGX_ERROR;
        }
    }

    name = ngx_array_push(keys);
    if (name == NULL) {
        return NGX_ERROR;
    }

    name->len = last - skip;
    name->data = ngx_pnalloc(ha->temp_pool, name->len);
    if (name->data == NULL) {
        return NGX_ERROR;
    }

    ngx_memcpy(name->data, key->data + skip, name->len);


    /* add to wildcard hash */

    hk = ngx_array_push(hwc);
    if (hk == NULL) {
        return NGX_ERROR;
    }

    hk->key.len = last - 1;
    hk->key.data = p;
    hk->key_hash = 0;
    hk->value = value;

    return NGX_OK;
}
