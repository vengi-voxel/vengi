/**
 * @file
 */

#include "Common.h"
#include "StandardLib.h"
#include <new>

void *operator new(std::size_t count) {
	return core_malloc(count);
}

void operator delete(void *ptr) noexcept {
	core_free(ptr);
}

void operator delete(void* ptr, std::size_t) {
	core_free(ptr);
}
