/**
 * @file
 */

#include "VolumeCompression.h"
#include "io/MemoryReadStream.h"
#include "io/ZipReadStream.h"

namespace voxel {

voxel::RawVolume *toVolume(const uint8_t *data, uint32_t dataSize, const voxel::Region &region) {
	const size_t uncompressedBufferSize = region.voxels() * sizeof(voxel::Voxel);
	io::MemoryReadStream dataStream(data, dataSize);
	io::ZipReadStream stream(dataStream, (int)dataStream.size());
	uint8_t *uncompressedBuf = (uint8_t *)core_malloc(uncompressedBufferSize);
	if (stream.read(uncompressedBuf, uncompressedBufferSize) == -1) {
		core_free(uncompressedBuf);
		return nullptr;
	}
	// don't pass const voxel::Voxel here - would create a memory leak - see the RawVolume ctors
	return voxel::RawVolume::createRaw((voxel::Voxel *)uncompressedBuf, region);
}

} // namespace voxel
