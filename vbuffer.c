/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

// size是每单元大小
void bfinit(struct buffer *pbuffer, size_t size) {
    memset(pbuffer, 0, sizeof *pbuffer);
    pbuffer->size = size;
    size_t newcap = BUFFER_INITIAL_CAPACITY;
    alloc_s((void **)&(pbuffer->base), pbuffer->capacity, newcap, pbuffer->size);
    pbuffer->capacity = newcap;
}

void *bfoffset(struct buffer *pbuffer, size_t index) {
    return (char *)pbuffer->base + index * pbuffer->size;
}

void bfclear(struct buffer *pbuffer) {
    exitif(pbuffer == NULL);
    free_s(&(pbuffer->base));
    pbuffer->capacity = 0;
    pbuffer->length = 0;
}

void bfstrip(struct buffer *pbuffer) {
    size_t newcap = 0; // 递减判断比较复杂
    while (newcap < pbuffer->length) {
        newcap = newcap == 0 ? 1 : newcap << 1;
        exitif(newcap == 0);
    }
    alloc_s((void **)&(pbuffer->base), pbuffer->capacity, newcap, pbuffer->size);
    pbuffer->capacity = newcap;
}

// bulk压栈
void bfpush(struct buffer *pbuffer, void *pvalues, size_t nvalues) {
    exitif(pbuffer == NULL);
    exitif(pvalues == NULL);
    size_t newqty = pbuffer->length + nvalues;
    if (pbuffer->capacity < newqty) { // 修正每次都调用alloc_s的逻辑错误
        size_t newcap = pbuffer->capacity;
        while (newcap < newqty) {
            newcap = newcap == 0 ? 1 : newcap << 1;
            exitif(newcap == 0);
        }
        alloc_s((void **)&(pbuffer->base), pbuffer->capacity, newcap, pbuffer->size);
        pbuffer->capacity = newcap;
    }
    memcpy(bfoffset(pbuffer, pbuffer->length), pvalues, nvalues * pbuffer->size);
    pbuffer->length = newqty;
}

#define bfpushall(pb, pv) bfpush(pb, pv, countof(pv))

// bulk出栈，注意出栈后的顺序和一个一个出栈的概念是不一样的
void bfpop(struct buffer *pbuffer, void *pvalues, size_t nvalues) {
    exitif(pbuffer == NULL);
    exitif(pbuffer->length < nvalues);
    exitif(pvalues == NULL);
    pbuffer->length -= nvalues;
    void *p = bfoffset(pbuffer, pbuffer->length);
    size_t nbytes = nvalues * pbuffer->size;
    memcpy(pvalues, p, nbytes);
    memset(p, 0, nbytes);
}

#define bfpopall(pb, pv) bfpop(pb, pv, (pb)->length)

void bfput(struct buffer *pbuffer, size_t index, void *pvalue) {
    exitif(pbuffer == NULL);
    exitif(index >= pbuffer->length);
    memcpy(bfoffset(pbuffer, index), pvalue, pbuffer->size);
}

void bfget(struct buffer *pbuffer, size_t index, void *pvalue) {
    exitif(pbuffer == NULL);
    exitif(index >= pbuffer->length);
    memcpy(pvalue, bfoffset(pbuffer, index), pbuffer->size);
}

void bfdump(struct buffer *pbuffer) {
    printf("    size: %lld\n", pbuffer->size);
    printf("capacity: %lld\n", pbuffer->capacity);
    printf("  length: %lld\n", pbuffer->length);
    char *addr = (char *)pbuffer->base;
    for (int i = 0; i < pbuffer->capacity; i++) {
        printf("%8d:", i);
        for (int j = 0; j < pbuffer->size; j++) {
            printf(" %02X", *addr);
            addr++;
        }
        printf("\n");
    }
    printf("\n");
}
