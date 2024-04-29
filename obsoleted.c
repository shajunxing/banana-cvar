/*
废弃的方案 abandoned obsoleted deprecated
*/

// 通用buffer类型
#define generic_buffer_struct_declaration(name, type) \
    struct name {                                     \
        type *base;                                   \
        size_t capacity;                              \
        size_t length;                                \
    }
#define generic_buffer_variable_declaration(sname, vname) struct sname vname = {NULL, 0, 0}
#define generic_buffer_clear_function_declaration(sname, fnprefix) \
    void fnprefix##clear(struct sname *pb)
#define generic_buffer_clear_function_definition(sname, fnprefix) \
    generic_buffer_clear_function_declaration(sname, fnprefix) {  \
        exitif(pb == NULL, EINVAL);                               \
        free_s((void **)&(pb->base));                             \
        pb->capacity = 0;                                         \
        pb->length = 0;                                           \
    }
#define generic_buffer_push_function_declaration(sname, fnprefix, type) \
    void fnprefix##push(struct sname *pb, type v)
#define generic_buffer_push_function_definition(sname, fnprefix, type)     \
    generic_buffer_push_function_declaration(sname, fnprefix, type) {      \
        exitif(pb == NULL, EINVAL);                                        \
        size_t newlen = pb->length + 1;                                    \
        size_t newcap = pb->capacity;                                      \
        while (newcap < newlen) {                                          \
            newcap = newcap == 0 ? 1 : newcap << 1;                        \
            exitif(newcap == 0, ERANGE);                                   \
        }                                                                  \
        alloc_s((void **)&(pb->base), pb->capacity, newcap, sizeof(type)); \
        pb->capacity = newcap;                                             \
        (pb->base)[pb->length] = v;                                        \
        pb->length = newlen;                                               \
    }
#define generic_buffer_pop_function_declaration(sname, fnprefix, type) \
    type fnprefix##pop(struct sname *pb)
#define generic_buffer_pop_function_definition(sname, fnprefix, type) \
    generic_buffer_pop_function_declaration(sname, fnprefix, type) {  \
        exitif(pb == NULL, EINVAL);                                   \
        exitif(pb->length == 0, ERANGE);                              \
        pb->length--;                                                 \
        type ret = (pb->base)[pb->length];                            \
        memset(&((pb->base)[pb->length]), 0, sizeof(type));           \
        return ret;                                                   \
    }
#define generic_buffer_put_function_declaration(sname, fnprefix, type) \
    void fnprefix##put(struct sname *pb, type v, size_t i)
#define generic_buffer_put_function_definition(sname, fnprefix, type) \
    generic_buffer_put_function_declaration(sname, fnprefix, type) {  \
        exitif(pb == NULL, EINVAL);                                   \
        exitif(i >= pb->length, ERANGE);                              \
        (pb->base)[i] = v;                                            \
    }
#define generic_buffer_get_function_declaration(sname, fnprefix, type) \
    type fnprefix##get(struct sname *pb, size_t i)
#define generic_buffer_get_function_definition(sname, fnprefix, type) \
    generic_buffer_get_function_declaration(sname, fnprefix, type) {  \
        exitif(pb == NULL, EINVAL);                                   \
        exitif(i >= pb->length, ERANGE);                              \
        return (pb->base)[i];                                         \
    }
// 查找失败返回-1
#define generic_buffer_find_function_declaration(sname, fnprefix, type) \
    ssize_t fnprefix##find(struct sname *pb, type v)
#define generic_buffer_find_function_definition(sname, fnprefix, type) \
    generic_buffer_find_function_declaration(sname, fnprefix, type) {  \
        exitif(pb == NULL, EINVAL);                                    \
        for (size_t i = 0; i < pb->length; i++) {                      \
            if (0 == memcmp(&((pb->base)[i]), &v, sizeof(type))) {     \
                return i;                                              \
            }                                                          \
        }                                                              \
        return -1;                                                     \
    }

#define aforeach(i, v, arr, stmt)                           \
    do {                                                    \
        exitif(arr == NULL, EINVAL);                        \
        exitif(arr->type != vtarray, EINVAL);               \
        for (size_t i = 0; i < (arr->avalue).length; i++) { \
            struct var *v = (arr->avalue).base[i];          \
            stmt;                                           \
        }                                                   \
    } while (0)

#define oforeach(k, klen, v, obj, stmt)                          \
    do {                                                         \
        exitif(obj == NULL, EINVAL);                             \
        exitif(obj->type != vtobject, EINVAL);                   \
        for (size_t _i = 0; _i < (obj->ovalue).capacity; _i++) { \
            char *k = (obj->ovalue).base[_i].key.base;           \
            size_t klen = (obj->ovalue).base[_i].key.length;     \
            struct var *v = (obj->ovalue).base[_i].value;        \
            if (k) {                                             \
                stmt;                                            \
            }                                                    \
        }                                                        \
    } while (0)
