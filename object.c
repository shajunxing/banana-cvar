/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

static void onbinit(struct objnodebuffer *pb) {
    memset(pb, 0, sizeof *pb);
    size_t newcap = BUFFER_INITIAL_CAPACITY;
    alloc_s((void **)&(pb->base), pb->capacity, newcap, sizeof(struct objnode));
    pb->capacity = newcap;
}

// 新建object
struct var *onew() {
    struct var *obj = vnew();
    obj->type = vtobject;
    onbinit(&(obj->ovalue));
    return obj;
}

static void onbfree(struct objnodebuffer *pb) {
    exitif(pb == NULL);
    free_s((void **)&(pb->base));
    pb->capacity = 0;
    pb->length = 0;
}

void oclear(struct var *obj) {
    exitif(obj == NULL);
    exitif(obj->type != vtobject);
    for (size_t i = 0; i < obj->ovalue.capacity; i++) {
        sbfree(&((obj->ovalue).base[i].key));
        (obj->ovalue).base[i].value = NULL;
    }
    (obj->ovalue).length = 0;
}

void ofree(struct var *obj) {
    exitif(obj == NULL);
    exitif(obj->type != vtobject);
    for (size_t i = 0; i < obj->ovalue.capacity; i++) {
        sbfree(&((obj->ovalue).base[i].key));
    }
    onbfree(&(obj->ovalue));
}

size_t olength(struct var *obj) {
    exitif(obj == NULL);
    exitif(obj->type != vtobject);
    return (obj->ovalue).length;
}

// first hash
static size_t fh(const char *s, size_t slen, size_t mask) {
    size_t hash = 0;
    for (size_t i = 0; i < slen; i++) {
        hash = (hash + (hash << 4) + s[i]) & mask;
    }
    return hash;
}

// next hash
static size_t nh(size_t hash, size_t mask) {
    // hash = hash * 5 + 1能完美全覆盖0-2的次方
    return (hash + (hash << 4) + 1) & mask;
}

/*
查找结果有以下一些可能性：
0、kv皆空
1、k不空v空，k同
2、k不空v空，k不同
3、kv皆不空，k同
4、kv皆不空，k不同
对于put，返回条件0、1、2、3
对于get，返回条件1、3
*/
enum findstatus {
    st_kv_empty,
    st_k_match_v_empty,
    st_k_nomatch_v_empty,
    st_k_match,
    st_k_nomatch,
    st_not_found,
};

enum forwhat {
    forput,
    forget,
};

struct findresult {
    enum findstatus stat;
    size_t hash;
    struct stringbuffer *pkey;
    struct var **pval;
};

// forwhat:
static struct findresult find(struct objnodebuffer *pb, const char *key, size_t klen, enum forwhat forwhat) {
    enum findstatus stat;
    size_t mask = pb->capacity - 1;
    size_t hash = fh(key, klen, mask);
    struct stringbuffer *pkey = NULL;
    struct var **pval = NULL;
    size_t rep = 0;
    for (; rep < pb->capacity; rep++) { // 最多重复cap次就能遍历整个数组
        // logdebug("key= %s, hash= %lld", key, hash);
        struct objnode *pnode = &((pb->base)[hash]);
        pkey = &(pnode->key);
        pval = &(pnode->value);
        bool kempty = pkey->base == NULL;
        bool vempty = *pval == NULL;
        exitif(kempty && !vempty); // 这是不应该发生的
        if (kempty) {
            stat = st_kv_empty;
        } else {
            bool kmatch = pkey->length == klen && memcmp(pkey->base, key, klen) == 0;
            if (vempty) {
                if (kmatch) {
                    stat = st_k_match_v_empty;
                } else {
                    stat = st_k_nomatch_v_empty;
                }
            } else {
                if (kmatch) {
                    stat = st_k_match;
                } else {
                    stat = st_k_nomatch;
                }
            }
        }
        if (forwhat == forput && stat >= st_kv_empty && stat <= st_k_match) {
            break;
        }
        if (forwhat == forget && (stat == st_k_match_v_empty || stat == st_k_match)) {
            break;
        }
        hash = nh(hash, mask);
    }
    if (rep >= pb->capacity) {
        stat = st_not_found;
        hash = 0;
        pkey = NULL;
        pval = NULL;
    }
    // logdebug("key= %s, forwhat= %d, stat= %d", key, forwhat, stat);
    return (struct findresult){stat, hash, pkey, pval};
}

/*
找不到空节点：增加（但是这不应该发生的，因为哈希表不应该满）
找到kv皆空的节点：增加（需要检查扩容）
找到k不空v空的节点，k不同：修改k，写入v，length++（需要检查扩容）
找到k不空v空的节点，k同：写入v，length++（需要检查扩容）
找到kv不空且k同的，相当于修改操作，length不变
*/
void oput_s(struct var *obj, const char *key, size_t klen, struct var *val) {
    exitif(obj == NULL);
    exitif(obj->type != vtobject);
    exitif(key == NULL);
    exitif(val == NULL);
    struct findresult ret = find(&(obj->ovalue), key, klen, forput);
    if (ret.stat == st_k_match) {
        *(ret.pval) = val;
    } else if (ret.stat == st_kv_empty || ret.stat == st_k_nomatch_v_empty || ret.stat == st_k_match_v_empty) {
        size_t newlen = (obj->ovalue).length + 1;
        size_t reqcap = newlen << 1; // 需要的容量至少是长度的2倍，测试时可设置为相等，以得到fsnotfound的状态
        if ((obj->ovalue).capacity >= reqcap) {
            // 无需扩容
            if (ret.stat == st_kv_empty) {
                sbappend_s(ret.pkey, key, klen);
            } else if (ret.stat == st_k_nomatch_v_empty) {
                sbclear(ret.pkey);
                sbappend_s(ret.pkey, key, klen);
            }
            *(ret.pval) = val;
            (obj->ovalue).length++;
        } else {
            size_t newcap = (obj->ovalue).capacity;
            do {
                newcap = newcap == 0 ? 1 : newcap << 1;
                exitif(newcap == 0);
            } while (newcap < reqcap);
            // logdebug("rehash, newlen= %lld, reqcap= %lld, newcap= %lld", newlen, reqcap, newcap);
            // 重新哈希
            struct objnodebuffer newoval = {NULL, 0, 0}; // 此处不调用onbinit
            alloc_s((void **)&(newoval.base), 0, newcap, sizeof(struct objnode));
            newoval.capacity = newcap;
            for (size_t i = 0; i < (obj->ovalue).capacity; i++) {
                struct objnode *pnode = &((obj->ovalue).base[i]);
                struct stringbuffer *pk = &(pnode->key);
                if (pk->base && pnode->value) { // 注意kv都不为NULL才是有效节点
                    struct findresult r = find(&newoval, pk->base, pk->length, forput);
                    exitif(r.stat != st_kv_empty); // 这是不应该发生的
                    newoval.base[r.hash] = *pnode;
                    newoval.length++;
                }
            }
            onbfree(&(obj->ovalue)); // 释放旧的objnodebuffer
            obj->ovalue = newoval; // newoval.address已托管，不需要释放
            // 再次查找
            struct findresult r = find(&(obj->ovalue), key, klen, forput);
            exitif(r.stat != st_kv_empty); // 这是不应该发生的
            sbappend_s(r.pkey, key, klen);
            *(r.pval) = val;
            (obj->ovalue).length++;
        }
    } else {
        logexit("ret.stat= %d shouldn't happen", ret.stat); // 这是不应该发生的
    }
}

void oput(struct var *obj, const char *key, struct var *val) {
    return oput_s(obj, key, strlen(key), val);
}

// 获取指向value的指针，在st_not_found时候该指针为NULL
// 当然该指针指向的value也可能是NULL
struct var **oget_p(struct var *obj, const char *key, size_t klen) {
    exitif(obj == NULL);
    exitif(obj->type != vtobject);
    exitif(key == NULL);
    struct findresult ret = find(&(obj->ovalue), key, klen, forget);
    return ret.pval;
}

// 返回NULL表示未找到，注意返回值务必检查NULL，否则别的以var*为参数的函数都会退出
struct var *oget_s(struct var *obj, const char *key, size_t klen) {
    struct var **pval = oget_p(obj, key, klen);
    return pval ? *pval : NULL;
}

struct var *oget(struct var *obj, const char *key) {
    return oget_s(obj, key, strlen(key));
}

// 直接针对value的指针做删除操作，也就是置NULL
// 这样在很多场合可以提高速度，但是暴露了细节，也有一定危险性
void odelete_p(struct var *obj, struct var **pval) {
    exitif(obj == NULL);
    exitif(obj->type != vtobject);
    if (pval && *pval) {
        *pval = NULL;
        (obj->ovalue).length--;
    }
}

// 删除操作把value置NULL，key不动，这样就不需要rehash了，确保了find的循环在后续key存在时候不会错误地断掉
void odelete_s(struct var *obj, const char *key, size_t klen) {
    odelete_p(obj, oget_p(obj, key, klen));
}

void odelete(struct var *obj, const char *key) {
    return odelete_s(obj, key, strlen(key));
}

void oforeach_p(struct var *obj, void (*cb)(const char *key, size_t klen, struct var **pval)) {
    exitif(obj == NULL);
    exitif(obj->type != vtobject);
    for (size_t i = 0; i < (obj->ovalue).capacity; i++) {
        char *k = (obj->ovalue).base[i].key.base;
        size_t klen = (obj->ovalue).base[i].key.length;
        struct var **p = &((obj->ovalue).base[i].value);
        if (k && *p) {
            // logdebug("k= %s, klen= %lld, v= %s", k, klen, tojson(v));
            cb(k, klen, p);
        }
    }
}

void oforeach(struct var *obj, void (*cb)(const char *key, size_t klen, struct var *val)) {
    void proxy(const char *key, size_t klen, struct var **pval) {
        cb(key, klen, *pval);
    }
    oforeach_p(obj, proxy);
}