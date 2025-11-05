/**
 * @file
 */

#include "engine-config.h" // USE_LZ4

#if USE_LZ4

#include "io/LZ4WriteStream.h"
namespace io {
using CompressionWriteStream = LZ4WriteStream;
}

#else

#if 0
#include "io/LZAVWriteStream.h"
namespace io {
using CompressionWriteStream = LZAVWriteStream;
}
#else
#include "io/ZipWriteStream.h"
namespace io {
using CompressionWriteStream = ZipWriteStream;
}
#endif

#endif
