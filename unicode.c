/*
Copyright 2024 ShaJunXing <shajunxing@hotmail.com>

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "var.h"

#define IO_CACHE_CAPACITY 1024

struct var *freadall(FILE *stream) {
    exitif(stream == NULL);
    struct var *result = vnew();
    result->type = vtstring;
    sbinit(&(result->svalue));
    char cache[IO_CACHE_CAPACITY];
    size_t nread;
    while ((nread = fread(cache, 1, sizeof cache, stream)) > 0) {
        sbappend_s(&((result)->svalue), cache, nread);
    }
    return result;
}

static inline int getstate(int ch) {
    switch (ch) {
    case '\r':
        return 1;
    case '\n':
        return 2;
    default:
        return 0;
    }
}

// 状态：0-普通字符，1-CR(\r,0x0D)，2-LF(\n,0x0A)
// 动作：0-什么都不做，1-设置起点，2-返回与起点之间的字符串，3-返回零长度字符串
static const int actiontable[3][3] = {
    {0, 2, 2},
    {1, 3, 0},
    {1, 3, 3},
};

void freadlines(FILE *stream, void (*cb)(const char *str, size_t slen)) {
    exitif(stream == NULL);
    exitif(cb == NULL);
    char cache[IO_CACHE_CAPACITY];
    struct stringbuffer line;
    sbinit(&line);
    int prevstate = 0;
    for (;;) {
        size_t nread = fread(cache, 1, sizeof cache, stream);
        if (0 == nread) {
            // logdebug("prevstate= %d", prevstate);
            cb(line.base, line.length);
            break;
        }
        char *from = cache;
        char *to = cache;
        char *cachend = cache + nread;
        for (; to < cachend; to++) {
            int state = getstate(*to);
            int action = actiontable[prevstate][state];
            // hexdump(from, to - from);
            // hexdump(line.base, line.length);
            // logdebug("prevstate= %d, state= %d, action= %d", prevstate, state, action);
            switch (action) {
            case 1:
                from = to;
                break;
            case 2:
                sbappend_s(&line, from, to - from);
                cb(line.base, line.length);
                sbclear(&line);
                break;
            case 3:
                cb(line.base, line.length);
                break;
            default:
                break;
            }
            prevstate = state;
        }
        // cache剩余未处理部分
        if (from < to) {
            // hexdump(from, to - from);
            // hexdump(line.base, line.length);
            // logdebug("prevstate= %d", prevstate);
            switch (prevstate) {
            case 0:
                sbappend_s(&line, from, to - from);
                break;
            default:
                break;
            }
        }
    }
    sbfree(&line);
}

// UTF8定义见
// https://en.wikipedia.org/wiki/UTF-8
// https://datatracker.ietf.org/doc/html/rfc3629

enum u8decoder_state {
    u8decoder_wait_initial,
    u8decoder_wait_following,
};

struct u8decoder {
    enum u8decoder_state state;
    uint8_t bytes[4];
    size_t nbytes_required;
    size_t nbytes_written;
    uint32_t output;
};

// 大于0为initial字节以及总字节数，-1为following字节，0为未知类型字节
static int check_u8byte(uint8_t byte) {
    if ((byte & 0b10000000) == 0) {
        return 1;
    } else if ((byte & 0b11000000) == 0b10000000) {
        return -1;
    } else if ((byte & 0b11100000) == 0b11000000) {
        return 2;
    } else if ((byte & 0b11110000) == 0b11100000) {
        return 3;
    } else if ((byte & 0b11111000) == 0b11110000) {
        return 4;
    } else {
        return 0;
    }
}

static uint32_t merge_u8bytes(uint8_t *bytes, size_t nbytes) {
    switch (nbytes) {
    case 1:
        return bytes[0] & 0b01111111;
    case 2:
        return ((bytes[0] & 0b00011111) << 6) | (bytes[1] & 0b00111111);
    case 3:
        return ((bytes[0] & 0b00001111) << 12) | ((bytes[1] & 0b00111111) << 6) | (bytes[2] & 0b00111111);
    case 4:
        return ((bytes[0] & 0b00000111) << 18) | ((bytes[1] & 0b00111111) << 12) | ((bytes[2] & 0b00111111) << 6) | (bytes[3] & 0b00111111);
    default:
        return 0;
    }
}

// unicode转换为utf8字节，buffer至少4字节，返回实际写入字节数（1-4）
static size_t encode_u32char(uint32_t unichar, uint8_t *buffer, size_t bufsz) {
    const uint32_t tab[][4] = {
        // {max cp + 1, num bytes, 1st byte shift, 1st byte mask}
        {0x80, 1, 0, 0},
        {0x800, 2, 6, 0b11000000},
        {0x10000, 3, 12, 0b11100000},
        {0x110000, 4, 18, 0b11110000},
    };
    size_t i, j, shift;
    for (i = 0; i < countof(tab); i++) {
        if (unichar < tab[i][0]) {
            // printf("[%d,%d,%d]", countof(tab), i, min(tab[i][1], bufsz));
            shift = tab[i][2];
            for (j = 0; j < min(tab[i][1], bufsz); j++) {
                if (0 == j) {
                    buffer[j] = (uint8_t)(unichar >> shift) | (uint8_t)tab[i][3];
                } else {
                    buffer[j] = ((uint8_t)(unichar >> shift) & 0b00111111) | 0b10000000;
                }
                shift -= 6;
            }
            return j;
        }
    }
    return 0; // 超出标准
}

static void init_u8decoder(struct u8decoder *fsm) {
    fsm->state = u8decoder_wait_initial;
    fsm->nbytes_written = 0;
}

// 输入字节，如果有输出则返回true
static int feed_u8decoder(struct u8decoder *fsm, uint8_t byte) {
    int byte_type = 0;
    byte_type = check_u8byte(byte);
    // printf("fsm->state=%d fsm->nbytes_written=%d byte=%X byte_type=%d\n", fsm->state, fsm->nbytes_written, byte, byte_type);
    if (byte_type > 0) {
        if (fsm->state == u8decoder_wait_following) {
            fsm->state = u8decoder_wait_initial;
        }
        fsm->nbytes_required = byte_type;
        fsm->bytes[0] = byte;
        fsm->nbytes_written = 1;
        if (fsm->nbytes_written < fsm->nbytes_required) {
            fsm->state = u8decoder_wait_following;
            return 0;
        } else {
            fsm->output = merge_u8bytes(fsm->bytes, fsm->nbytes_written);
            fsm->state = u8decoder_wait_initial;
            return 1;
        }
    } else if (fsm->state == u8decoder_wait_initial) {
        return 0;
    } else if (byte_type == -1) {
        fsm->bytes[fsm->nbytes_written] = byte;
        fsm->nbytes_written++;
        if (fsm->nbytes_written < fsm->nbytes_required) {
            return 0;
        } else {
            fsm->output = merge_u8bytes(fsm->bytes, fsm->nbytes_written);
            fsm->state = u8decoder_wait_initial;
            return 1;
        }
    } else {
        fsm->state = u8decoder_wait_initial;
        return 0;
    }
}

// 注意无法做出freadunichars(buffer, len)的形式，除非去掉cache
void freadunichars(FILE *stream, void (*cb)(uint32_t unichar)) {
    exitif(stream == NULL);
    exitif(cb == NULL);
    char cache[IO_CACHE_CAPACITY];
    struct u8decoder fsm;
    init_u8decoder(&fsm);
    size_t nbytes_read = 0;
    size_t nbytes_fed = 0;
    for (;;) {
        if (nbytes_fed < nbytes_read) {
            if (feed_u8decoder(&fsm, cache[nbytes_fed++])) {
                cb(fsm.output);
            }
        } else {
            nbytes_fed = 0;
            nbytes_read = fread(cache, 1, sizeof cache, stream);
            // printf("nbytes_read=%d\n", nbytes_read);
            if (0 == nbytes_read) {
                break;
            }
        }
    }
}

size_t fwriteunichars(FILE *stream, const uint32_t *unichars, size_t nchars) {
    exitif(stream == NULL);
    size_t nchars_written = 0;
    for (size_t i = 0; i < nchars; i++) {
        uint8_t buffer[4];
        size_t nbytes = encode_u32char(unichars[i], buffer, sizeof buffer);
        if (nbytes && nbytes == fwrite(buffer, 1, nbytes, stream)) {
            nchars_written++;
        }
    }
    return nchars_written;
}

size_t fwriteunichar(FILE *stream, uint32_t unichar) {
    return fwriteunichars(stream, &unichar, 1);
}
