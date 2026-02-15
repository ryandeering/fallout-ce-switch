#ifndef FALLOUT_PLIB_GNW_MEMORY_H_
#define FALLOUT_PLIB_GNW_MEMORY_H_

#include <stddef.h>

namespace fallout {

typedef void*(MallocFunc)(size_t size);
typedef void*(ReallocFunc)(void* ptr, size_t newSize);
typedef void(FreeFunc)(void* ptr);

char* mem_strdup(const char* string);
void* mem_malloc(size_t size);
void* mem_realloc(void* ptr, size_t size);
void mem_free(void* ptr);
void mem_check();
void mem_register_func(MallocFunc* mallocFunc, ReallocFunc* reallocFunc, FreeFunc* freeFunc);
size_t mem_get_allocated();
size_t mem_get_peak_allocated();
int mem_get_block_count();
int mem_get_peak_block_count();

} // namespace fallout

#endif /* FALLOUT_PLIB_GNW_MEMORY_H_ */
