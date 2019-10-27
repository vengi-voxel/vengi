/**
 * @file
 */

#include "Common.h"
#include "Trace.h"

namespace core {

#ifdef TRACY_ENABLED
void* operator new(std::size_t count) {
	auto ptr = core_malloc(count);
	TracyAlloc(ptr , count);
	return ptr;
}

void operator delete (void* ptr) noexcept {
	TracyFree(ptr);
	core_free(ptr);
}
#endif

}
