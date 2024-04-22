/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

// 新建object
struct var *onew() {
    struct var *pv = vnew();
    pv->type = vtobject;
    return pv;
}

static generic_buffer_clear_function_definition(objnodebuffer, onb);

void oclear(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtobject, EINVAL);
    for (size_t i = 0; i < pv->ovalue.capacity; i++) {
        sbclear(&((pv->ovalue).address[i].key));
    }
    onbclear(&(pv->ovalue));
}

size_t olength(struct var *pv) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtobject, EINVAL);
    return (pv->ovalue).length;
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

enum findstatus {
    fsemptynode,
    fskeymatch,
    fsnotfound,
};

struct findresult {
    enum findstatus stat;
    size_t hash;
};

static struct findresult find(struct objnodebuffer *pb, const char *key, size_t klen) {
    size_t mask = pb->capacity - 1;
    size_t hash = fh(key, klen, mask);
    // printf("key=%s\n", key);
    for (size_t rep = 0; rep < pb->capacity; rep++) { // 最多重复cap次就能遍历整个数组
        // printf("rep=%d,hash=%d\n", rep, hash);
        struct objnode *pon = &((pb->address)[hash]);
        struct stringbuffer *pkey = &(pon->key);
        struct var *value = pon->value;
        if (pkey->address == NULL) {
            return (struct findresult){.stat = fsemptynode, .hash = hash};
        } else if ((pkey->length == klen) && (memcmp(pkey->address, key, klen) == 0)) {
            return (struct findresult){.stat = fskeymatch, .hash = hash};
        }
        hash = nh(hash, mask);
    }
    return (struct findresult){.stat = fsnotfound, .hash = hash};
}

void oput_s(struct var *pv, const char *key, size_t klen, struct var *pval) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtobject, EINVAL);
    exitif(key == NULL, EINVAL);
    exitif(pval == NULL, EINVAL);
    // 先查找key存在与否
    struct findresult ret = find(&(pv->ovalue), key, klen);
    if (ret.stat == fskeymatch) {
        // 存在则修改值并返回
        (pv->ovalue).address[ret.hash].value = pval;
        return;
    }
    // 不存在则增加
    size_t newlen = (pv->ovalue).length + 1;
    size_t reqcap = newlen << 1; // 需要的容量至少是长度的2倍，测试时可设置为相等，以得到fsnotfound的状态
    if ((pv->ovalue).capacity >= reqcap) {
        // 无需扩容
        exitif(ret.stat != fsemptynode, ERANGE); // 这是不应该发生的
        sbappend_s(&((pv->ovalue).address[ret.hash].key), key, klen);
        (pv->ovalue).address[ret.hash].value = pval;
        (pv->ovalue).length++;
    } else {
        size_t newcap = (pv->ovalue).capacity;
        do {
            newcap = newcap == 0 ? 1 : newcap << 1;
            exitif(newcap == 0, ERANGE);
        } while (newcap < reqcap);
        // 重新哈希
        generic_buffer_variable_declaration(objnodebuffer, newoval);
        alloc_s((void **)&(newoval.address), 0, newcap, sizeof(struct objnode));
        newoval.capacity = newcap;
        for (size_t i = 0; i < (pv->ovalue).capacity; i++) {
            struct objnode node = (pv->ovalue).address[i];
            if (node.key.address) { // 该节点是存在的
                struct findresult ret = find(&newoval, node.key.address, node.key.length);
                exitif(ret.stat != fsemptynode, ERANGE); // 这是不应该发生的
                newoval.address[ret.hash] = node;
                newoval.length++;
            }
        }
        onbclear(&(pv->ovalue)); // 释放旧的objnodebuffer
        pv->ovalue = newoval; // newoval.address已托管，不需要释放
        // 再次查找
        struct findresult ret = find(&(pv->ovalue), key, klen);
        exitif(ret.stat != fsemptynode, ERANGE); // 这是不应该发生的
        sbappend_s(&((pv->ovalue).address[ret.hash].key), key, klen);
        (pv->ovalue).address[ret.hash].value = pval;
        (pv->ovalue).length++;
    }
}

void oput(struct var *pv, const char *key, struct var *pval) {
    return oput_s(pv, key, strlen(key), pval);
}

// 返回NULL表示未找到，注意返回值务必检查NULL，否则别的以var*为参数的函数都会退出
struct var *oget_s(struct var *pv, const char *key, size_t klen) {
    exitif(pv == NULL, EINVAL);
    exitif(pv->type != vtobject, EINVAL);
    exitif(key == NULL, EINVAL);
    struct findresult ret = find(&(pv->ovalue), key, klen);
    if (ret.stat == fskeymatch) {
        return (pv->ovalue).address[ret.hash].value;
    } else {
        return NULL;
    }
}

struct var *oget(struct var *pv, const char *key) {
    return oget_s(pv, key, strlen(key));
}

void oforeach(struct var *obj, void (*cb)(const char *k, size_t klen, struct var *v)) {
    exitif(obj == NULL, EINVAL);
    exitif(obj->type != vtobject, EINVAL);
    for (size_t _i = 0; _i < (obj->ovalue).capacity; _i++) {
        char *k = (obj->ovalue).address[_i].key.address;
        size_t klen = (obj->ovalue).address[_i].key.length;
        struct var *v = (obj->ovalue).address[_i].value;
        if (k) {
            cb(k, klen, v);
        }
    }
}