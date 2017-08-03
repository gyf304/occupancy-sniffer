#ifndef _BLOOM_H
#define _BLOOM_H

#include "os_type.h"

typedef struct bloom_header* bloom_t;

bool bloom_create(bloom_t* bloom_ptr, uint32_t buf_size, uint8_t hash_viarations);
bool bloom_add(bloom_t b, uint8_t* x, uint32_t elem_size);
bool bloom_is_in(bloom_t b, uint8_t* x, uint32_t elem_size);
bool bloom_destroy(bloom_t b);

#endif