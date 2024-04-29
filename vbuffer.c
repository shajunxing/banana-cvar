/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

void *bfoffset(struct buffer *pbuffer, size_t index) {
    return (char *)pbuffer->base + index * pbuffer->size;
}

void bfclear(struct buffer *pbuffer) {
    exitif(pbuffer == NULL, EINVAL);
    free_s(&(pbuffer->base));
    pbuffer->capacity = 0;
    pbuffer->length = 0;
}

void bfstrip(struct buffer *pbuffer) {
    size_t newcap = 0; // 递减判断比较复杂
    while (newcap < pbuffer->length) {
        newcap = newcap == 0 ? 1 : newcap << 1;
        exitif(newcap == 0, ERANGE);
    }
    alloc_s((void **)&(pbuffer->base), pbuffer->capacity, newcap, pbuffer->size);
    pbuffer->capacity = newcap;
}

// bulk压栈
void bfpush(struct buffer *pbuffer, void *pvalues, size_t nvalues) {
    exitif(pbuffer == NULL, EINVAL);
    exitif(pvalues == NULL, EINVAL);
    size_t newqty = pbuffer->length + nvalues;
    size_t newcap = pbuffer->capacity;
    while (newcap < newqty) {
        newcap = newcap == 0 ? 1 : newcap << 1;
        exitif(newcap == 0, ERANGE);
    }
    alloc_s((void **)&(pbuffer->base), pbuffer->capacity, newcap, pbuffer->size);
    pbuffer->capacity = newcap;
    memcpy(bfoffset(pbuffer, pbuffer->length), pvalues, nvalues * pbuffer->size);
    pbuffer->length = newqty;
}

#define bfpushall(pb, pv) bfpush(pb, pv, countof(pv))

// bulk出栈，注意出栈后的顺序和一个一个出栈的概念是不一样的
void bfpop(struct buffer *pbuffer, void *pvalues, size_t nvalues) {
    exitif(pbuffer == NULL, EINVAL);
    exitif(pbuffer->length < nvalues, ERANGE);
    exitif(pvalues == NULL, EINVAL);
    pbuffer->length -= nvalues;
    void *p = bfoffset(pbuffer, pbuffer->length);
    size_t nbytes = nvalues * pbuffer->size;
    memcpy(pvalues, p, nbytes);
    memset(p, 0, nbytes);
}

#define bfpopall(pb, pv) bfpop(pb, pv, (pb)->length)

void bfput(struct buffer *pbuffer, size_t index, void *pvalue) {
    exitif(pbuffer == NULL, EINVAL);
    exitif(index >= pbuffer->length, ERANGE);
    memcpy(bfoffset(pbuffer, index), pvalue, pbuffer->size);
}

void bfget(struct buffer *pbuffer, size_t index, void *pvalue) {
    exitif(pbuffer == NULL, EINVAL);
    exitif(index >= pbuffer->length, ERANGE);
    memcpy(pvalue, bfoffset(pbuffer, index), pbuffer->size);
}

void bfdump(struct buffer *pbuffer) {
    printf("    size: %d\n", pbuffer->size);
    printf("capacity: %d\n", pbuffer->capacity);
    printf("  length: %d\n", pbuffer->length);
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
