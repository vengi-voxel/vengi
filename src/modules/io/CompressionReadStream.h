/**
 * @file
 */

#include "engine-config.h" // USE_LZ4

#if USE_LZ4

#include "io/LZ4ReadStream.h"
namespace io {
using CompressionReadStream = LZ4ReadStream;
}

#else

#if 0
#include "io/LZAVReadStream.h"
namespace io {
using CompressionReadStream = LZAVReadStream;
}
#else
#include "io/ZipReadStream.h"
namespace io {
using CompressionReadStream = ZipReadStream;
}
#endif

#endif
