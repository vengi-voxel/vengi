/**
 * @file
 */

#include "PagedVolume.h"
#include "Morton.h"
#include "Utility.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/Trace.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/round.hpp>

namespace voxel {

/**
 * This constructor creates a volume with a fixed size which is specified as a parameter. By default this constructor will not enable paging
 * but you can override this if desired. If you do wish to enable
 * paging then you are required to provide the call back function (see the other PagedVolume constructor).
 * @param pager Called by PolyVox to load and unload data on demand.
 * @param targetMemoryUsageInBytes The upper limit to how much memory this PagedVolume should aim to use.
 * @param chunkSideLength The size of the chunks making up the volume. Small chunks will compress/decompress faster, but there will also be
 * more of them meaning voxel access could be slower.
 */
PagedVolume::PagedVolume(Pager* pager, uint32_t targetMemoryUsageInBytes, uint16_t chunkSideLength) :
		_chunkSideLength(chunkSideLength), _pager(pager), _region(0, 0, 0, -1, -1, -1) {
	// Validation of parameters
	core_assert_msg(_pager, "You must provide a valid pager when constructing a PagedVolume");
	core_assert_msg(targetMemoryUsageInBytes >= 1 * 1024 * 1024, "Target memory usage is too small to be practical");
	core_assert_msg(_chunkSideLength != 0, "Chunk side length cannot be zero.");
	core_assert_msg(_chunkSideLength <= 256, "Chunk size is too large to be practical.");
	core_assert_msg(glm::isPowerOfTwo(_chunkSideLength), "Chunk side length must be a power of two.");

	// Used to perform multiplications and divisions by bit shifting.
	_chunkSideLengthPower = logBase2(_chunkSideLength);
	// Use to perform modulo by bit operations
	_chunkMask = _chunkSideLength - 1;

	// Calculate the number of chunks based on the memory limit and the size of each chunk.
	uint32_t chunkSizeInBytes = PagedVolume::Chunk::calculateSizeInBytes(_chunkSideLength);
	_chunkCountLimit = targetMemoryUsageInBytes / chunkSizeInBytes;

	// Enforce sensible limits on the number of chunks.
	const uint32_t minPracticalNoOfChunks = 32; // Enough to make sure a chunks and it's neighbours can be loaded, with a few to spare.
	if (_chunkCountLimit < minPracticalNoOfChunks) {
		Log::warn("Requested memory usage limit of %uMb is too low and cannot be adhered to. Chunk limit is at %i, Chunk size: %uKb",
				targetMemoryUsageInBytes / (1024 * 1024), _chunkCountLimit, chunkSizeInBytes / 1024);
	}
	_chunkCountLimit = core_max(_chunkCountLimit, minPracticalNoOfChunks);

	// Inform the user about the chosen memory configuration.
	Log::info("Memory usage limit for volume now set to %uMb (%u chunks of %uKb each).",
			(_chunkCountLimit * chunkSizeInBytes) / (1024 * 1024), _chunkCountLimit, chunkSizeInBytes / 1024);
}

/**
 * Destroys the volume The destructor will call flushAll() to ensure that a paging volume has the chance to save it's
 * data via the dataOverflowHandler() if desired.
 */
PagedVolume::~PagedVolume() {
	flushAll();
}

/**
 * This version of the function is provided so that the wrap mode does not need
 * to be specified as a template parameter, as it may be confusing to some users.
 * @param uXPos The @c x position of the voxel
 * @param uYPos The @c y position of the voxel
 * @param uZPos The @c z position of the voxel
 * @return The voxel value
 */
const Voxel& PagedVolume::voxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const {
	return voxel(glm::ivec3(uXPos, uYPos, uZPos));
}

PagedVolume::ChunkPtr PagedVolume::chunk(const glm::ivec3& pos) const {
	const int32_t chunkX = pos.x >> _chunkSideLengthPower;
	const int32_t chunkY = pos.y >> _chunkSideLengthPower;
	const int32_t chunkZ = pos.z >> _chunkSideLengthPower;
	return chunk(chunkX, chunkY, chunkZ);
}

/**
 * This version of the function is provided so that the wrap mode does not need
 * to be specified as a template parameter, as it may be confusing to some users.
 * @param v3dPos The 3D position of the voxel
 * @return The voxel value
 */
const Voxel& PagedVolume::voxel(const glm::ivec3& v3dPos) const {
	const uint16_t xOffset = static_cast<uint16_t>(v3dPos.x & _chunkMask);
	const uint16_t yOffset = static_cast<uint16_t>(v3dPos.y & _chunkMask);
	const uint16_t zOffset = static_cast<uint16_t>(v3dPos.z & _chunkMask);
	return chunk(v3dPos)->voxel(xOffset, yOffset, zOffset);
}

/**
 * @param uXPos the @c x position of the voxel
 * @param uYPos the @c y position of the voxel
 * @param uZPos the @c z position of the voxel
 */
void PagedVolume::setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, const Voxel& tValue) {
	if (!_region.isValid()) {
		_region = Region(uXPos, uYPos, uZPos, uXPos, uYPos, uZPos);
	} else {
		_region.accumulate(uXPos, uYPos, uZPos);
	}
	const int32_t chunkX = uXPos >> _chunkSideLengthPower;
	const int32_t chunkY = uYPos >> _chunkSideLengthPower;
	const int32_t chunkZ = uZPos >> _chunkSideLengthPower;

	const uint16_t xOffset = static_cast<uint16_t>(uXPos - (chunkX << _chunkSideLengthPower));
	const uint16_t yOffset = static_cast<uint16_t>(uYPos - (chunkY << _chunkSideLengthPower));
	const uint16_t zOffset = static_cast<uint16_t>(uZPos - (chunkZ << _chunkSideLengthPower));

	chunk(chunkX, chunkY, chunkZ)->setVoxel(xOffset, yOffset, zOffset, tValue);
}

/**
 * @param v3dPos the 3D position of the voxel
 * @param tValue the value to which the voxel will be set
 */
void PagedVolume::setVoxel(const glm::ivec3& v3dPos, const Voxel& tValue) {
	setVoxel(v3dPos.x, v3dPos.y, v3dPos.z, tValue);
}

void PagedVolume::setVoxels(int32_t uXPos, int32_t uZPos, const Voxel* tArray, int amount) {
	setVoxels(uXPos, 0, uZPos, 1, 1, tArray, amount);
}

glm::ivec3 PagedVolume::chunkPos(int x, int y, int z) const {
	const int32_t _x = x >> _chunkSideLengthPower;
	const int32_t _y = y >> _chunkSideLengthPower;
	const int32_t _z = z >> _chunkSideLengthPower;
	return glm::ivec3(_x, _y, _z);
}

void PagedVolume::setVoxels(int32_t uXPos, int32_t uYPos, int32_t uZPos, int nx, int nz, const Voxel* tArray, int amount) {
	core_trace_scoped(VolumeSetVoxels);
	if (!_region.isValid()) {
		_region = Region(uXPos, uYPos, uZPos, uXPos + nx, uYPos, uZPos + nz);
	} else {
		_region.accumulate(Region(uXPos, uYPos, uZPos, uXPos + nx, uYPos, uZPos + nz));
	}
	for (int x = uXPos; x < uXPos + nx; ++x) {
		const int32_t chunkX = x >> _chunkSideLengthPower;
		const uint16_t xOffset = static_cast<uint16_t>(x & _chunkMask);
		for (int z = uZPos; z < uZPos + nz; ++z) {
			int32_t y = uYPos;
			int left = amount;
			const Voxel* array = tArray;
			const int32_t chunkZ = z >> _chunkSideLengthPower;
			const uint16_t zOffset = static_cast<uint16_t>(z & _chunkMask);
			while (left > 0) {
				const int32_t chunkY = y >> _chunkSideLengthPower;
				const uint16_t yOffset = static_cast<uint16_t>(y & _chunkMask);

				ChunkPtr chunkPtr = chunk(chunkX, chunkY, chunkZ);
				const int32_t n = core_min(left, int32_t(chunkPtr->_sideLength));

				chunkPtr->setVoxels(xOffset, yOffset, zOffset, array, n);
				left -= n;
				array += ptrdiff_t(n);
				y += n;
			}
		}
	}
}

/**
 * Removes all voxels from memory by removing all chunks. The application has the chance to persist the data via @c Pager::pageOut
 */
void PagedVolume::flushAll() {
	core::ScopedWriteLock writeLock(_volumeLock);
	_chunks.clear();
}

/**
 * As we have added a chunk we may have exceeded our target chunk limit. Search through the array to
 * determine how many chunks we have, as well as finding the oldest timestamp. Note that this is potentially
 * wasteful and we may instead wish to track how many chunks we have and/or delete a chunk at random (or
 * just check e.g. 10 and delete the oldest of those) but we'll see if this is a bottleneck first. Paging
 * the data in is probably more expensive.
 */
void PagedVolume::deleteOldestChunkIfNeeded() const {
	core_trace_scoped(DeleteOldestChunk);
	ChunkMap::iterator oldestChunk = _chunks.end();
	uint32_t oldestChunkTimestamp = _timestamper;
	for (ChunkMap::iterator i = _chunks.begin(); i != _chunks.end(); ++i) {
		const ChunkPtr& chunk = i->second;
		if (chunk->_chunkLastAccessed < oldestChunkTimestamp) {
			oldestChunkTimestamp = chunk->_chunkLastAccessed;
			oldestChunk = i;
		}
	}
	if (oldestChunk != _chunks.end()) {
		Log::debug("delete oldest chunk - reached %u", _chunkCountLimit);
		_chunks.erase(oldestChunk);
	}
}

PagedVolume::ChunkPtr PagedVolume::createNewChunk(int32_t chunkX, int32_t chunkY, int32_t chunkZ) const {
	core_trace_scoped(CreateNewChunk);
	// The chunk was not found so we will create a new one.
	glm::ivec3 pos(chunkX, chunkY, chunkZ);
	Log::debug("create new chunk at %i:%i:%i", chunkX, chunkY, chunkZ);
	ChunkPtr chunk = core::make_shared<Chunk>(pos, _chunkSideLength, _pager);
	chunk->_chunkLastAccessed = ++_timestamper; // Important, as we may soon delete the oldest chunk

	// Pass the chunk to the Pager to give it a chance to initialise it with any data
	// From the coordinates of the chunk we deduce the coordinates of the contained voxels.
	PagerContext pctx;
	const glm::ivec3& mins = pos * static_cast<int32_t>(_chunkSideLength);
	const glm::ivec3& maxs = mins + glm::ivec3(_chunkSideLength - 1);
	pctx.region = Region(mins, maxs);
	pctx.chunk = chunk;

	// Page the data in
	// We'll use this later to decide if data needs to be paged out again.
	chunk->_dataModified = _pager->pageIn(pctx);
	Log::debug("finished creating new chunk at %i:%i:%i", chunkX, chunkY, chunkZ);

	return chunk;
}

PagedVolume::ChunkPtr PagedVolume::chunk(int32_t chunkX, int32_t chunkY, int32_t chunkZ) const {
	core_trace_scoped(PagedVolumeChunk);
	core::ScopedWriteLock chunkWriteLock(_volumeLock);
	const glm::ivec3 pos(chunkX, chunkY, chunkZ);
	auto i = _chunks.find(pos);
	if (i == _chunks.end()) {
		const ChunkPtr& chunk = createNewChunk(chunkX, chunkY, chunkZ);
		_chunks.put(pos, chunk);
		const std::size_t chunkCount = _chunks.size();
		if (chunkCount >= _chunkCountLimit) {
			deleteOldestChunkIfNeeded();
		}
		return chunk;
	}
	const ChunkPtr& chunk = i->second;
	chunk->_chunkLastAccessed = ++_timestamper;
	return chunk;
}

}
