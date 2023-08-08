/**
 * @file
 */

#include "Zip.h"
#include "Log.h"
#include "Assert.h"
extern "C" {
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES 1
#include "external/miniz.h"
}

namespace core {
namespace zip {

uint32_t compressBound(uint32_t in) {
	core_assert_msg(in > 0, "Expected to get a size > 0 - but got %i", (int)in);
	return (uint32_t)::mz_compressBound((mz_ulong)in);
}

}
}
