/**
 * @file
 */

#include "PagedVolume.h"
#include "Morton.h"
#include "Utility.h"

namespace voxel {

/**
 * This constructor creates a volume with a fixed size which is specified as a parameter. By default this constructor will not enable paging
 * but you can override this if desired. If you do wish to enable
 * paging then you are required to provide the call back function (see the other PagedVolume constructor).
 * @param pPager Called by PolyVox to load and unload data on demand.
 * @param uTargetMemoryUsageInBytes The upper limit to how much memory this PagedVolume should aim to use.
 * @param uChunkSideLength The size of the chunks making up the volume. Small chunks will compress/decompress faster, but there will also be
 * more of them meaning voxel access could be slower.
 */
PagedVolume::PagedVolume(Pager* pPager, uint32_t uTargetMemoryUsageInBytes, uint16_t uChunkSideLength) :
		_chunkSideLength(uChunkSideLength), _pager(pPager) {
	// Validation of parameters
	core_assert_msg(pPager, "You must provide a valid pager when constructing a PagedVolume");
	core_assert_msg(uTargetMemoryUsageInBytes >= 1 * 1024 * 1024, "Target memory usage is too small to be practical");
	core_assert_msg(_chunkSideLength != 0, "Chunk side length cannot be zero.");
	core_assert_msg(_chunkSideLength <= 256, "Chunk size is too large to be practical.");
	core_assert_msg(glm::isPowerOfTwo(_chunkSideLength), "Chunk side length must be a power of two.");

	// Used to perform multiplications and divisions by bit shifting.
	_chunkSideLengthPower = logBase2(_chunkSideLength);
	// Use to perform modulo by bit operations
	_chunkMask = _chunkSideLength - 1;

	// Calculate the number of chunks based on the memory limit and the size of each chunk.
	uint32_t uChunkSizeInBytes = PagedVolume::Chunk::calculateSizeInBytes(_chunkSideLength);
	_chunkCountLimit = uTargetMemoryUsageInBytes / uChunkSizeInBytes;

	// Enforce sensible limits on the number of chunks.
	const uint32_t uMinPracticalNoOfChunks = 32; // Enough to make sure a chunks and it's neighbours can be loaded, with a few to spare.
	if (_chunkCountLimit < uMinPracticalNoOfChunks) {
		Log::warn("Requested memory usage limit of %uMb is too low and cannot be adhered to. Chunk limit is at %i, Chunk size: %uKb",
				uTargetMemoryUsageInBytes / (1024 * 1024), _chunkCountLimit, uChunkSizeInBytes / 1024);
	}
	_chunkCountLimit = std::max(_chunkCountLimit, uMinPracticalNoOfChunks);

	// Inform the user about the chosen memory configuration.
	Log::debug("Memory usage limit for volume now set to %uMb (%u chunks of %uKb each).",
			(_chunkCountLimit * uChunkSizeInBytes) / (1024 * 1024), _chunkCountLimit, uChunkSizeInBytes / 1024);
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
const Voxel& PagedVolume::getVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const {
	return getVoxel(glm::ivec3(uXPos, uYPos, uZPos));
}

PagedVolume::ChunkPtr PagedVolume::getChunk(const glm::ivec3& pos) const {
	const int32_t chunkX = pos.x >> _chunkSideLengthPower;
	const int32_t chunkY = pos.y >> _chunkSideLengthPower;
	const int32_t chunkZ = pos.z >> _chunkSideLengthPower;
	return getChunk(chunkX, chunkY, chunkZ);
}

/**
 * This version of the function is provided so that the wrap mode does not need
 * to be specified as a template parameter, as it may be confusing to some users.
 * @param v3dPos The 3D position of the voxel
 * @return The voxel value
 */
const Voxel& PagedVolume::getVoxel(const glm::ivec3& v3dPos) const {
	const uint16_t xOffset = static_cast<uint16_t>(v3dPos.x & _chunkMask);
	const uint16_t yOffset = static_cast<uint16_t>(v3dPos.y & _chunkMask);
	const uint16_t zOffset = static_cast<uint16_t>(v3dPos.z & _chunkMask);
	return getChunk(v3dPos)->getVoxel(xOffset, yOffset, zOffset);
}

/**
 * @param uXPos the @c x position of the voxel
 * @param uYPos the @c y position of the voxel
 * @param uZPos the @c z position of the voxel
 */
void PagedVolume::setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, const Voxel& tValue) {
	const int32_t chunkX = uXPos >> _chunkSideLengthPower;
	const int32_t chunkY = uYPos >> _chunkSideLengthPower;
	const int32_t chunkZ = uZPos >> _chunkSideLengthPower;

	const uint16_t xOffset = static_cast<uint16_t>(uXPos - (chunkX << _chunkSideLengthPower));
	const uint16_t yOffset = static_cast<uint16_t>(uYPos - (chunkY << _chunkSideLengthPower));
	const uint16_t zOffset = static_cast<uint16_t>(uZPos - (chunkZ << _chunkSideLengthPower));

	getChunk(chunkX, chunkY, chunkZ)->setVoxel(xOffset, yOffset, zOffset, tValue);
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

void PagedVolume::setVoxels(int32_t uXPos, int32_t uYPos, int32_t uZPos, int nx, int nz, const Voxel* tArray, int amount) {
	for (int j = 0; j < nx; ++j) {
		for (int k = 0; k < nz; ++k) {
			int32_t y = uYPos;
			const int32_t x = uXPos + j;
			const int32_t z = uZPos + k;
			int left = amount;
			const Voxel* array = tArray;
			const int32_t chunkX = x >> _chunkSideLengthPower;
			const int32_t chunkZ = z >> _chunkSideLengthPower;
			const uint16_t xOffset = static_cast<uint16_t>(x & _chunkMask);
			const uint16_t zOffset = static_cast<uint16_t>(z & _chunkMask);
			while (left > 0) {
				const int32_t chunkY = y >> _chunkSideLengthPower;
				const uint16_t yOffset = static_cast<uint16_t>(y & _chunkMask);

				ChunkPtr chunk = getChunk(chunkX, chunkY, chunkZ);
				const int32_t n = std::min(left, int32_t(chunk->_sideLength));

				chunk->setVoxels(xOffset, yOffset, zOffset, array, n);
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
	core::RecursiveScopedWriteLock writeLock(_rwLock);
	// Clear this pointer as all chunks are about to be removed.
	_lastAccessedChunk = ChunkPtr();

	// Erase all the most recently used chunks.
	for (ChunkMap::iterator i = _chunks.begin(); i != _chunks.end(); ++i) {
	}
	_chunks.clear();
}

/**
 * Starting at the position indicated by the hash, and then search through the whole array looking for a chunk with the correct
 * position. In most cases we expect to find it in the first place we look. Note that this algorithm is slow in the case that
 * the chunk is not found because the whole array has to be searched, but in this case we are going to have to page the data in
 * from an external source which is likely to be slow anyway.
 */
PagedVolume::ChunkPtr PagedVolume::getExistingChunk(int32_t chunkX, int32_t chunkY, int32_t chunkZ) const {
	const glm::ivec3 pos(chunkX, chunkY, chunkZ);
	auto i = _chunks.find(pos);
	if (i == _chunks.end()) {
		return nullptr;
	}
	const PagedVolume::ChunkPtr& chunk = i->second;
	chunk->_chunkLastAccessed = ++_timestamper;
	return chunk;
}

/**
 * As we have added a chunk we may have exceeded our target chunk limit. Search through the array to
 * determine how many chunks we have, as well as finding the oldest timestamp. Note that this is potentially
 * wasteful and we may instead wish to track how many chunks we have and/or delete a chunk at random (or
 * just check e.g. 10 and delete the oldest of those) but we'll see if this is a bottleneck first. Paging
 * the data in is probably more expensive.
 */
void PagedVolume::deleteOldestChunkIfNeeded() const {
	const std::size_t chunkCount = _chunks.size();
	if (chunkCount < _chunkCountLimit) {
		return;
	}
	glm::ivec3 oldestChunkPos(glm::uninitialize);
	bool foundOldestChunk = false;
	uint32_t oldestChunkTimestamp = std::numeric_limits<uint32_t>::max();
	for (ChunkMap::iterator i = _chunks.begin(); i != _chunks.end(); ++i) {
		const glm::ivec3& pos = i->first;
		const ChunkPtr& chunk = i->second;
		if (chunk->_chunkLastAccessed < oldestChunkTimestamp) {
			oldestChunkTimestamp = chunk->_chunkLastAccessed;
			foundOldestChunk = true;
			oldestChunkPos = pos;
		}
	}
	if (foundOldestChunk) {
		ChunkMap::iterator i = _chunks.find(oldestChunkPos);
		_chunks.erase(i);
	}
}

PagedVolume::ChunkPtr PagedVolume::createNewChunk(int32_t chunkX, int32_t chunkY, int32_t chunkZ) const {
	// The chunk was not found so we will create a new one.
	glm::ivec3 pos(chunkX, chunkY, chunkZ);
	Log::debug("create new chunk at %i:%i:%i", chunkX, chunkY, chunkZ);
	ChunkPtr chunk = std::make_shared<Chunk>(pos, _chunkSideLength, _pager);
	chunk->_chunkLastAccessed = ++_timestamper; // Important, as we may soon delete the oldest chunk

	{
		core::RecursiveScopedWriteLock volumeWriteLock(_rwLock);
		auto i = _chunks.insert(std::make_pair(pos, chunk));
		if (!i.second) {
			return i.first->second;
		}
		deleteOldestChunkIfNeeded();
	}


	// Pass the chunk to the Pager to give it a chance to initialise it with any data
	// From the coordinates of the chunk we deduce the coordinates of the contained voxels.
	const glm::ivec3 mins = chunk->_chunkSpacePosition * static_cast<int32_t>(chunk->_sideLength);
	const glm::ivec3 maxs = mins + glm::ivec3(chunk->_sideLength - 1, chunk->_sideLength - 1, chunk->_sideLength - 1);

	PagerContext pctx;
	pctx.region = Region(mins, maxs);
	pctx.chunk = chunk;

	// Page the data in
	// We'll use this later to decide if data needs to be paged out again.
	core::RecursiveScopedWriteLock chunkWriteLock(chunk->_rwLock);
	chunk->_dataModified = _pager->pageIn(pctx);
	Log::debug("finished creating new chunk at %i:%i:%i", chunkX, chunkY, chunkZ);

	return chunk;
}

PagedVolume::ChunkPtr PagedVolume::getChunk(int32_t chunkX, int32_t chunkY, int32_t chunkZ) const {
	ChunkPtr chunk;
	{
		core::RecursiveScopedReadLock readLock(_rwLock);
		if (chunkX == _lastAccessedChunkX && chunkY == _lastAccessedChunkY && chunkZ == _lastAccessedChunkZ && _lastAccessedChunk) {
			return _lastAccessedChunk;
		}
		chunk = getExistingChunk(chunkX, chunkY, chunkZ);
	}

	// If we still haven't found the chunk then it's time to create a new one and page it in from disk.
	if (!chunk) {
		chunk = createNewChunk(chunkX, chunkY, chunkZ);
	}

	core::RecursiveScopedWriteLock writeLock(_rwLock);
	_lastAccessedChunk = chunk;
	_lastAccessedChunkX = chunkX;
	_lastAccessedChunkY = chunkY;
	_lastAccessedChunkZ = chunkZ;

	return chunk;
}

/**
 * Calculate the memory usage of the volume.
 */
uint32_t PagedVolume::calculateSizeInBytes() {
	core::RecursiveScopedReadLock readLock(_rwLock);
	const std::size_t uChunkCount = _chunks.size();
	// Note: We disregard the size of the other class members as they are likely to be very small compared to the size of the
	// allocated voxel data. This also keeps the reported size as a power of two, which makes other memory calculations easier.
	return PagedVolume::Chunk::calculateSizeInBytes(_chunkSideLength) * uChunkCount;
}

PagedVolume::Chunk::Chunk(glm::ivec3 v3dPosition, uint16_t uSideLength, Pager* pPager) :
		_pager(pPager), _chunkSpacePosition(v3dPosition) {
	core_assert_msg(_pager, "No valid pager supplied to chunk constructor.");
	core_assert_msg(uSideLength <= 256, "Chunk side length cannot be greater than 256.");

	// Compute the side length
	_sideLength = uSideLength;
	_sideLengthPower = logBase2(uSideLength);

	// Allocate the data
	const uint32_t uNoOfVoxels = _sideLength * _sideLength * _sideLength;
	_data = new Voxel[uNoOfVoxels];
}

PagedVolume::Chunk::~Chunk() {
	if (_dataModified && _pager) {
		_pager->pageOut(this);
	}

	delete[] _data;
	_data = nullptr;
}

bool PagedVolume::Chunk::isGenerated() const {
	return _dataModified;
}

Voxel* PagedVolume::Chunk::getData() const {
	return _data;
}

uint32_t PagedVolume::Chunk::getDataSizeInBytes() const {
	return _sideLength * _sideLength * _sideLength * sizeof(Voxel);
}

const Voxel& PagedVolume::Chunk::getVoxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos) const {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path
	core_assert_msg(uXPos < _sideLength, "Supplied position is outside of the chunk. asserted %u > %u", uXPos, _sideLength);
	core_assert_msg(uYPos < _sideLength, "Supplied position is outside of the chunk. asserted %u > %u", uYPos, _sideLength);
	core_assert_msg(uZPos < _sideLength, "Supplied position is outside of the chunk. asserted %u > %u", uZPos, _sideLength);
	core_assert_msg(_data, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	const uint32_t index = morton256_x[uXPos] | morton256_y[uYPos] | morton256_z[uZPos];
	core::RecursiveScopedReadLock readLock(_rwLock);
	return _data[index];
}

const Voxel& PagedVolume::Chunk::getVoxel(const glm::i16vec3& v3dPos) const {
	return getVoxel(v3dPos.x, v3dPos.y, v3dPos.z);
}

void PagedVolume::Chunk::setVoxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos, const Voxel& tValue) {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path
	core_assert_msg(uXPos < _sideLength, "Supplied position is outside of the chunk");
	core_assert_msg(uYPos < _sideLength, "Supplied position is outside of the chunk");
	core_assert_msg(uZPos < _sideLength, "Supplied position is outside of the chunk");
	core_assert_msg(_data, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	const uint32_t index = morton256_x[uXPos] | morton256_y[uYPos] | morton256_z[uZPos];
	_data[index] = tValue;
	_dataModified = true;
}

void PagedVolume::Chunk::setVoxels(uint32_t uXPos, uint32_t uZPos, const Voxel* tValues, int amount) {
	setVoxels(uXPos, 0, uZPos, tValues, amount);
}

void PagedVolume::Chunk::setVoxels(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos, const Voxel* tValues, int amount) {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path
	core_assert_msg(amount <= _sideLength, "Supplied amount exceeds chunk boundaries");
	core_assert_msg(uXPos < _sideLength, "Supplied x position is outside of the chunk");
	core_assert_msg(uYPos < _sideLength, "Supplied y position is outside of the chunk");
	core_assert_msg(uZPos < _sideLength, "Supplied z position is outside of the chunk");
	core_assert_msg(_data, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	for (int y = uYPos; y < amount; ++y) {
		const uint32_t index = morton256_x[uXPos] | morton256_y[y] | morton256_z[uZPos];
		_data[index] = tValues[y];
	}
	_dataModified = true;
}

void PagedVolume::Chunk::setVoxel(const glm::i16vec3& v3dPos, const Voxel& tValue) {
	setVoxel(v3dPos.x, v3dPos.y, v3dPos.z, tValue);
}

uint32_t PagedVolume::Chunk::calculateSizeInBytes() const {
	// Call through to the static version
	return calculateSizeInBytes(_sideLength);
}

uint32_t PagedVolume::Chunk::calculateSizeInBytes(uint32_t uSideLength) {
	// Note: We disregard the size of the other class members as they are likely to be very small compared to the size of the
	// allocated voxel data. This also keeps the reported size as a power of two, which makes other memory calculations easier.
	const uint32_t uSizeInBytes = uSideLength * uSideLength * uSideLength * sizeof(Voxel);
	return uSizeInBytes;
}

#define CAN_GO_NEG_X(val) (val > 0)
#define CAN_GO_POS_X(val)  (val < this->_chunkSideLengthMinusOne)
#define CAN_GO_NEG_Y(val) (val > 0)
#define CAN_GO_POS_Y(val)  (val < this->_chunkSideLengthMinusOne)
#define CAN_GO_NEG_Z(val) (val > 0)
#define CAN_GO_POS_Z(val)  (val < this->_chunkSideLengthMinusOne)

#define NEG_X_DELTA (-(deltaX[this->_xPosInChunk-1]))
#define POS_X_DELTA (deltaX[this->_xPosInChunk])
#define NEG_Y_DELTA (-(deltaY[this->_yPosInChunk-1]))
#define POS_Y_DELTA (deltaY[this->_yPosInChunk])
#define NEG_Z_DELTA (-(deltaZ[this->_zPosInChunk-1]))
#define POS_Z_DELTA (deltaZ[this->_zPosInChunk])

PagedVolume::Sampler::Sampler(const PagedVolume* volume) :
		_volume(volume), _xPosInVolume(0), _yPosInVolume(0), _zPosInVolume(0), _chunkSideLengthMinusOne(
				volume->_chunkSideLength - 1) {
}

PagedVolume::Sampler::Sampler(const PagedVolume& volume) :
		_volume(&volume), _xPosInVolume(0), _yPosInVolume(0), _zPosInVolume(0), _chunkSideLengthMinusOne(
				volume._chunkSideLength - 1) {
}

PagedVolume::Sampler::~Sampler() {
}

void PagedVolume::Sampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	_xPosInVolume = xPos;
	_yPosInVolume = yPos;
	_zPosInVolume = zPos;

	// Then we update the voxel pointer
	const int32_t xChunk = _xPosInVolume >> _volume->_chunkSideLengthPower;
	const int32_t yChunk = _yPosInVolume >> _volume->_chunkSideLengthPower;
	const int32_t zChunk = _zPosInVolume >> _volume->_chunkSideLengthPower;

	_xPosInChunk = static_cast<uint16_t>(_xPosInVolume - (xChunk << _volume->_chunkSideLengthPower));
	_yPosInChunk = static_cast<uint16_t>(_yPosInVolume - (yChunk << _volume->_chunkSideLengthPower));
	_zPosInChunk = static_cast<uint16_t>(_zPosInVolume - (zChunk << _volume->_chunkSideLengthPower));

	const uint32_t voxelIndexInChunk = morton256_x[_xPosInChunk] | morton256_y[_yPosInChunk] | morton256_z[_zPosInChunk];

	_currentChunk = _volume->getChunk(xChunk, yChunk, zChunk);
	_currentVoxel = _currentChunk->_data + voxelIndexInChunk;
}

bool PagedVolume::Sampler::setVoxel(const Voxel& tValue) {
	if (_currentVoxel == nullptr) {
		return false;
	}
	//Need to think what effect this has on any existing iterators.
	//core_assert_msg(false, "This function cannot be used on PagedVolume samplers.");
	*_currentVoxel = tValue;
	return true;
}

void PagedVolume::Sampler::movePositiveX() {
	_xPosInVolume++;

	// Then we update the voxel pointer
	if (CAN_GO_POS_X(_xPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += POS_X_DELTA;
		_xPosInChunk++;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

void PagedVolume::Sampler::movePositiveY() {
	_yPosInVolume++;

	// Then we update the voxel pointer
	if (CAN_GO_POS_Y(_yPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += POS_Y_DELTA;
		_yPosInChunk++;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

void PagedVolume::Sampler::movePositiveZ() {
	_zPosInVolume++;

	// Then we update the voxel pointer
	if (CAN_GO_POS_Z(_zPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += POS_Z_DELTA;
		_zPosInChunk++;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

void PagedVolume::Sampler::moveNegativeX() {
	_xPosInVolume--;

	// Then we update the voxel pointer
	if (CAN_GO_NEG_X(_xPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += NEG_X_DELTA;
		_xPosInChunk--;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

void PagedVolume::Sampler::moveNegativeY() {
	_yPosInVolume--;

	// Then we update the voxel pointer
	if (CAN_GO_NEG_Y(_yPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += NEG_Y_DELTA;
		_yPosInChunk--;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

void PagedVolume::Sampler::moveNegativeZ() {
	_zPosInVolume--;

	// Then we update the voxel pointer
	if (CAN_GO_NEG_Z(_zPosInChunk)) {
		//No need to compute new chunk.
		_currentVoxel += NEG_Z_DELTA;
		_zPosInChunk--;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume);
	}
}

}
