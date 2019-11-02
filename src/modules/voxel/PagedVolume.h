/**
 * @file
 */

#pragma once

#include "Voxel.h"
#include "Region.h"
#include "core/NonCopyable.h"
#include "core/RecursiveReadWriteLock.h"
#include <array>
#include <memory>
#include <atomic>
#include <list>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace voxel {

/**
 * This class provide a volume implementation which avoids storing all the data in memory at all times. Instead it breaks the volume
 * down into a set of chunks and moves these into and out of memory on demand. This means it is much more memory efficient than the
 * RawVolume, but may also be slower and is more complicated We encourage uses to work with RawVolume initially, and then switch to
 * PagedVolume once they have a larger application and/or a better understanding of PolyVox.
 *
 * The PagedVolume makes use of a Pager which defines the source and/or destination for data paged into and out of memory. PolyVox
 * comes with an example FilePager though users can also implement their own approaches. For example, the Pager could instead stream
 * data from a network connection or generate it procedurally on demand.
 *
 * A consequence of this paging approach is that (unlike the RawVolume) the PagedVolume does not need to have a predefined size. After
 * the volume has been created you can begin accessing voxels anywhere in space and the required data will be created automatically.
 */
class PagedVolume: public core::NonCopyable {
	friend class PagedVolumeWrapper;
public:
	/// The PagedVolume stores it data as a set of Chunk instances which can be loaded and unloaded as memory requirements dictate.
	class Chunk;
	/// The Pager class is responsible for the loading and unloading of Chunks, and can be subclassed by the user.
	class Pager;

	class Chunk : public std::enable_shared_from_this<Chunk> {
		friend class PagedVolume;
		friend class PagedVolumeWrapper;

	public:
		Chunk(const glm::ivec3& v3dPosition, uint16_t uSideLength, Pager* pPager = nullptr);
		~Chunk();

		Voxel* data() const;
		uint32_t dataSizeInBytes() const;

		bool containsPoint(const glm::ivec3& pos) const;
		bool containsPoint(int32_t x, int32_t y, int32_t z) const;
		Region region() const;

		const Voxel& voxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos) const;
		const Voxel& voxel(const glm::i16vec3& v3dPos) const;

		void setVoxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos, const Voxel& tValue);
		void setVoxels(uint32_t uXPos, uint32_t uZPos, const Voxel* tValues, int amount);
		void setVoxels(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos, const Voxel* tValues, int amount);
		void setVoxel(const glm::i16vec3& v3dPos, const Voxel& tValue);

	private:
		// This is updated by the PagedVolume and used to discard the least recently used chunks.
		uint32_t _chunkLastAccessed = 0u;

		uint32_t calculateSizeInBytes() const;
		static uint32_t calculateSizeInBytes(uint32_t uSideLength);

		Voxel* _data = nullptr;
		uint16_t _sideLength = 0u;

		// This is so we can tell whether a uncompressed chunk has to be recompressed and whether
		// a compressed chunk has to be paged back to disk, or whether they can just be discarded.
		bool _dataModified = false;

		uint8_t _sideLengthPower = 0b0;
		Pager* _pager;

		// Note: Do we really need to store this position here as well as in the block maps?
		glm::ivec3 _chunkSpacePosition;

		core::RecursiveReadWriteLock _rwLock{"chunk"};
	};
	typedef std::shared_ptr<Chunk> ChunkPtr;

	struct IChunkListener {
		virtual ~IChunkListener() {}
		virtual void onCreate(const ChunkPtr& ptr) {};

		virtual void onRemove(const ChunkPtr& ptr) {};
	};

	struct PagerContext {
		Region region;
		ChunkPtr chunk;
	};

	/**
	 * Users can override this class and provide an instance of the derived class to the PagedVolume constructor. This derived class
	 * could then perform tasks such as compression and decompression of the data, and read/writing it to a file, database, network,
	 * or other storage as appropriate. See FilePager for a simple example of such a derived class.
	 */
	class Pager {
	public:
		/// Constructor
		Pager() {
		}

		/// Destructor
		virtual ~Pager() {
		}

		/**
		 * @return @c true if the chunk was modified (created), @c false if it was just loaded
		 */
		virtual bool pageIn(PagerContext& ctx) = 0;
		virtual void pageOut(Chunk* chunk) = 0;
	};

	class Sampler {
	public:
		Sampler(const PagedVolume* volume);
		Sampler(const PagedVolume& volume);
		virtual ~Sampler();

		const Voxel& voxel() const;

		inline bool currentPositionValid() const {
			return true;
		}

		void setPosition(const glm::ivec3& v3dNewPos);
		virtual void setPosition(int32_t xPos, int32_t yPos, int32_t zPos);
		bool setVoxel(const Voxel& tValue);
		glm::ivec3 position() const;

		void movePositiveX();
		void movePositiveY();
		void movePositiveZ();

		void moveNegativeX();
		void moveNegativeY();
		void moveNegativeZ();

		const Voxel& peekVoxel1nx1ny1nz() const;
		const Voxel& peekVoxel1nx1ny0pz() const;
		const Voxel& peekVoxel1nx1ny1pz() const;
		const Voxel& peekVoxel1nx0py1nz() const;
		const Voxel& peekVoxel1nx0py0pz() const;
		const Voxel& peekVoxel1nx0py1pz() const;
		const Voxel& peekVoxel1nx1py1nz() const;
		const Voxel& peekVoxel1nx1py0pz() const;
		const Voxel& peekVoxel1nx1py1pz() const;

		const Voxel& peekVoxel0px1ny1nz() const;
		const Voxel& peekVoxel0px1ny0pz() const;
		const Voxel& peekVoxel0px1ny1pz() const;
		const Voxel& peekVoxel0px0py1nz() const;
		const Voxel& peekVoxel0px0py0pz() const;
		const Voxel& peekVoxel0px0py1pz() const;
		const Voxel& peekVoxel0px1py1nz() const;
		const Voxel& peekVoxel0px1py0pz() const;
		const Voxel& peekVoxel0px1py1pz() const;

		const Voxel& peekVoxel1px1ny1nz() const;
		const Voxel& peekVoxel1px1ny0pz() const;
		const Voxel& peekVoxel1px1ny1pz() const;
		const Voxel& peekVoxel1px0py1nz() const;
		const Voxel& peekVoxel1px0py0pz() const;
		const Voxel& peekVoxel1px0py1pz() const;
		const Voxel& peekVoxel1px1py1nz() const;
		const Voxel& peekVoxel1px1py0pz() const;
		const Voxel& peekVoxel1px1py1pz() const;

	protected:
		const PagedVolume* _volume;

		//The current position in the volume
		int32_t _xPosInVolume;
		int32_t _yPosInVolume;
		int32_t _zPosInVolume;

		//Other current position information
		Voxel* _currentVoxel = nullptr;
		ChunkPtr _currentChunk	;

		uint16_t _xPosInChunk = 0u;
		uint16_t _yPosInChunk = 0u;
		uint16_t _zPosInChunk = 0u;

		// This should ideally be const, but that prevent automatic generation of an assignment operator (https://goo.gl/Sn7KpZ).
		// We could provide one manually, but it's currently unused so there is no real test for if it works. I'm putting
		// together a new release at the moment so I'd rathern not make 'risky' changes.
		uint16_t _chunkSideLengthMinusOne;
	};

	/**
	 * The buffered sampler will extract the given region into a local buffer
	 * and operates on that one - and not on the volume data. This means that
	 * there is no thread locking involved while operating on the data.
	 */
	class BufferedSampler {
	public:
		BufferedSampler(const PagedVolume* volume, const Region& region);
		BufferedSampler(const PagedVolume& volume, const Region& region);
		~BufferedSampler();

		const Voxel& voxel() const;

		bool setPosition(const glm::ivec3& v3dNewPos);
		bool setPosition(int32_t xPos, int32_t yPos, int32_t zPos);
		bool setVoxel(const Voxel& tValue);
		glm::ivec3 position() const;

		void movePositiveX();
		void movePositiveY();
		void movePositiveZ();

		void moveNegativeX();
		void moveNegativeY();
		void moveNegativeZ();

		const Voxel& peekVoxel1nx1ny1nz() const;
		const Voxel& peekVoxel1nx1ny0pz() const;
		const Voxel& peekVoxel1nx1ny1pz() const;
		const Voxel& peekVoxel1nx0py1nz() const;
		const Voxel& peekVoxel1nx0py0pz() const;
		const Voxel& peekVoxel1nx0py1pz() const;
		const Voxel& peekVoxel1nx1py1nz() const;
		const Voxel& peekVoxel1nx1py0pz() const;
		const Voxel& peekVoxel1nx1py1pz() const;

		const Voxel& peekVoxel0px1ny1nz() const;
		const Voxel& peekVoxel0px1ny0pz() const;
		const Voxel& peekVoxel0px1ny1pz() const;
		const Voxel& peekVoxel0px0py1nz() const;
		const Voxel& peekVoxel0px0py0pz() const;
		const Voxel& peekVoxel0px0py1pz() const;
		const Voxel& peekVoxel0px1py1nz() const;
		const Voxel& peekVoxel0px1py0pz() const;
		const Voxel& peekVoxel0px1py1pz() const;

		const Voxel& peekVoxel1px1ny1nz() const;
		const Voxel& peekVoxel1px1ny0pz() const;
		const Voxel& peekVoxel1px1ny1pz() const;
		const Voxel& peekVoxel1px0py1nz() const;
		const Voxel& peekVoxel1px0py0pz() const;
		const Voxel& peekVoxel1px0py1pz() const;
		const Voxel& peekVoxel1px1py1nz() const;
		const Voxel& peekVoxel1px1py0pz() const;
		const Voxel& peekVoxel1px1py1pz() const;

		inline const uint8_t* data() const {
			return (const uint8_t*)_buffer.data();
		}

	protected:
		uint16_t index(int x, int y, int z) const;

		std::vector<Voxel> _buffer;

		//The current position in the volume
		int32_t _xPosInVolume = 0u;
		int32_t _yPosInVolume = 0u;
		int32_t _zPosInVolume = 0u;

		//Other current position information
		Voxel* _currentVoxel = nullptr;

		uint16_t _xPosInBuffer = 0u;
		uint16_t _yPosInBuffer = 0u;
		uint16_t _zPosInBuffer = 0u;

		uint16_t _regionWidth;
		uint16_t _regionHeight;
		uint16_t _regionDepth;
		uint16_t _zOffset;

		int32_t _minsX;
		int32_t _minsY;
		int32_t _minsZ;
	};

public:
	/// Constructor for creating a fixed size volume.
	PagedVolume(Pager* pager, uint32_t targetMemoryUsageInBytes = 256 * 1024 * 1024, uint16_t chunkSideLength = 32);
	/// Destructor
	~PagedVolume();

	/// Gets a voxel at the position given by <tt>x,y,z</tt> coordinates
	const Voxel& voxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const;
	/// Gets a voxel at the position given by a 3D vector
	const Voxel& voxel(const glm::ivec3& v3dPos) const;

	void addChunkListener(IChunkListener* listener);
	void removeChunkListener(IChunkListener* listener);

	const Region& region() const;

	/// Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	void setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, const Voxel& tValue);
	/// Sets the voxel at the position given by a 3D vector
	void setVoxel(const glm::ivec3& v3dPos, const Voxel& tValue);
	/// Sets the voxel at the position given by <tt>x,z</tt> coordinates
	void setVoxels(int32_t uXPos, int32_t uZPos, const Voxel* tArray, int amount);
	void setVoxels(int32_t uXPos, int32_t uYPos, int32_t uZPos, int nx, int nz, const Voxel* tArray, int amount);

	/// Removes all voxels from memory
	void flushAll();

	/// Calculates approximately how many bytes of memory the volume is currently using.
	uint32_t calculateSizeInBytes();
	ChunkPtr chunk(const glm::ivec3& pos) const;
	bool hasChunk(const glm::ivec3& pos) const;

	inline uint16_t chunkSideLength() const {
		return _chunkSideLength;
	}

protected:
	/// Copy constructor
	PagedVolume(const PagedVolume& rhs);

	/// Assignment operator
	PagedVolume& operator=(const PagedVolume& rhs);

	std::list<IChunkListener*> _listener;

private:
	ChunkPtr chunk(int32_t uChunkX, int32_t uChunkY, int32_t uChunkZ) const;
	ChunkPtr existingChunk(int32_t uChunkX, int32_t uChunkY, int32_t uChunkZ) const;
	ChunkPtr createNewChunk(int32_t uChunkX, int32_t uChunkY, int32_t uChunkZ) const;
	void deleteOldestChunkIfNeeded() const;

	// Storing these properties individually has proved to be faster than keeping
	// them in a glm::ivec3 as it avoids constructions and comparison overheads.
	// They are also at the start of the class in the hope that they will be pulled
	// into cache - I've got no idea if this actually makes a difference.
	mutable int32_t _lastAccessedChunkX = 0;
	mutable int32_t _lastAccessedChunkY = 0;
	mutable int32_t _lastAccessedChunkZ = 0;
	mutable ChunkPtr _lastAccessedChunk = nullptr;

	mutable std::atomic_uint _timestamper { 0u };

	uint32_t _chunkCountLimit = 0u;

	typedef std::unordered_map<glm::ivec3, ChunkPtr, std::hash<glm::ivec3> > ChunkMap;
	mutable ChunkMap _chunks;

	// The size of the chunks
	uint16_t _chunkSideLength;
	uint8_t _chunkSideLengthPower;
	int32_t _chunkMask;

	Pager* _pager = nullptr;

	Region _region;

	mutable core::RecursiveReadWriteLock _rwLock{"pagedvolume"};
	mutable core::RecursiveReadWriteLock _listenerLock{"listener"};
};

inline const Voxel& PagedVolume::Sampler::voxel() const {
	return *_currentVoxel;
}

inline void PagedVolume::Sampler::setPosition(const glm::ivec3& v3dNewPos) {
	setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
}

inline glm::ivec3 PagedVolume::Sampler::position() const {
	return glm::ivec3(_xPosInVolume, _yPosInVolume, _zPosInVolume);
}

// These precomputed offset are used to determine how much we move our pointer by to move a single voxel in the x, y, or z direction given an x, y, or z starting position inside a chunk.
// More information in this discussion: https://bitbucket.org/volumesoffun/polyvox/issue/61/experiment-with-morton-ordering-of-voxel
static const std::array<int32_t, 256> deltaX = {{ 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1, 3511, 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1, 28087, 1, 7, 1, 55, 1, 7,
		1, 439, 1, 7, 1, 55, 1, 7, 1, 3511, 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1, 224695, 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1, 3511, 1, 7, 1, 55, 1, 7, 1,
		439, 1, 7, 1, 55, 1, 7, 1, 28087, 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1, 3511, 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1, 1797559, 1, 7, 1, 55, 1, 7, 1,
		439, 1, 7, 1, 55, 1, 7, 1, 3511, 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1, 28087, 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1, 3511, 1, 7, 1, 55, 1, 7, 1, 439,
		1, 7, 1, 55, 1, 7, 1, 224695, 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1, 3511, 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1, 28087, 1, 7, 1, 55, 1, 7, 1, 439, 1,
		7, 1, 55, 1, 7, 1, 3511, 1, 7, 1, 55, 1, 7, 1, 439, 1, 7, 1, 55, 1, 7, 1 }};
static const std::array<int32_t, 256> deltaY = {{ 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 7022, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 56174, 2, 14,
		2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 7022, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 449390, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2,
		7022, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 56174, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 7022, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2,
		110, 2, 14, 2, 3595118, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 7022, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 56174, 2, 14, 2, 110, 2, 14, 2,
		878, 2, 14, 2, 110, 2, 14, 2, 7022, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 449390, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 7022, 2, 14, 2,
		110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 56174, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2, 7022, 2, 14, 2, 110, 2, 14, 2, 878, 2, 14, 2, 110, 2, 14, 2 }};
static const std::array<int32_t, 256> deltaZ = {{ 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 14044, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 112348, 4,
		28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 14044, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 898780, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4,
		28, 4, 14044, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 112348, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 14044, 4, 28, 4, 220, 4, 28, 4, 1756,
		4, 28, 4, 220, 4, 28, 4, 7190236, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 14044, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 112348, 4, 28, 4,
		220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 14044, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 898780, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4,
		14044, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 112348, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28, 4, 220, 4, 28, 4, 14044, 4, 28, 4, 220, 4, 28, 4, 1756, 4, 28,
		4, 220, 4, 28, 4 }};

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

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1ny1nz() const {
	if (CAN_GO_NEG_X(this->_xPosInChunk) && CAN_GO_NEG_Y(this->_yPosInChunk) && CAN_GO_NEG_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + NEG_X_DELTA + NEG_Y_DELTA + NEG_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume - 1, this->_yPosInVolume - 1, this->_zPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1ny0pz() const {
	if (CAN_GO_NEG_X(this->_xPosInChunk) && CAN_GO_NEG_Y(this->_yPosInChunk)) {
		return *(_currentVoxel + NEG_X_DELTA + NEG_Y_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume - 1, this->_yPosInVolume - 1, this->_zPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1ny1pz() const {
	if (CAN_GO_NEG_X(this->_xPosInChunk) && CAN_GO_NEG_Y(this->_yPosInChunk) && CAN_GO_POS_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + NEG_X_DELTA + NEG_Y_DELTA + POS_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume - 1, this->_yPosInVolume - 1, this->_zPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx0py1nz() const {
	if (CAN_GO_NEG_X(this->_xPosInChunk) && CAN_GO_NEG_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + NEG_X_DELTA + NEG_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume - 1, this->_yPosInVolume, this->_zPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx0py0pz() const {
	if (CAN_GO_NEG_X(this->_xPosInChunk)) {
		return *(_currentVoxel + NEG_X_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume - 1, this->_yPosInVolume, this->_zPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx0py1pz() const {
	if (CAN_GO_NEG_X(this->_xPosInChunk) && CAN_GO_POS_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + NEG_X_DELTA + POS_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume - 1, this->_yPosInVolume, this->_zPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1py1nz() const {
	if (CAN_GO_NEG_X(this->_xPosInChunk) && CAN_GO_POS_Y(this->_yPosInChunk) && CAN_GO_NEG_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + NEG_X_DELTA + POS_Y_DELTA + NEG_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume - 1, this->_yPosInVolume + 1, this->_zPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1py0pz() const {
	if (CAN_GO_NEG_X(this->_xPosInChunk) && CAN_GO_POS_Y(this->_yPosInChunk)) {
		return *(_currentVoxel + NEG_X_DELTA + POS_Y_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume - 1, this->_yPosInVolume + 1, this->_zPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1py1pz() const {
	if (CAN_GO_NEG_X(this->_xPosInChunk) && CAN_GO_POS_Y(this->_yPosInChunk) && CAN_GO_POS_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + NEG_X_DELTA + POS_Y_DELTA + POS_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume - 1, this->_yPosInVolume + 1, this->_zPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1ny1nz() const {
	if (CAN_GO_NEG_Y(this->_yPosInChunk) && CAN_GO_NEG_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + NEG_Y_DELTA + NEG_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume, this->_yPosInVolume - 1, this->_zPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1ny0pz() const {
	if (CAN_GO_NEG_Y(this->_yPosInChunk)) {
		return *(_currentVoxel + NEG_Y_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume, this->_yPosInVolume - 1, this->_zPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1ny1pz() const {
	if (CAN_GO_NEG_Y(this->_yPosInChunk) && CAN_GO_POS_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + NEG_Y_DELTA + POS_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume, this->_yPosInVolume - 1, this->_zPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px0py1nz() const {
	if (CAN_GO_NEG_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + NEG_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume, this->_yPosInVolume, this->_zPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px0py0pz() const {
	return *_currentVoxel;
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px0py1pz() const {
	if (CAN_GO_POS_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + POS_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume, this->_yPosInVolume, this->_zPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1py1nz() const {
	if (CAN_GO_POS_Y(this->_yPosInChunk) && CAN_GO_NEG_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + POS_Y_DELTA + NEG_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume, this->_yPosInVolume + 1, this->_zPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1py0pz() const {
	if (CAN_GO_POS_Y(this->_yPosInChunk)) {
		return *(_currentVoxel + POS_Y_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume, this->_yPosInVolume + 1, this->_zPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1py1pz() const {
	if (CAN_GO_POS_Y(this->_yPosInChunk) && CAN_GO_POS_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + POS_Y_DELTA + POS_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume, this->_yPosInVolume + 1, this->_zPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1ny1nz() const {
	if (CAN_GO_POS_X(this->_xPosInChunk) && CAN_GO_NEG_Y(this->_yPosInChunk) && CAN_GO_NEG_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + POS_X_DELTA + NEG_Y_DELTA + NEG_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume + 1, this->_yPosInVolume - 1, this->_zPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1ny0pz() const {
	if (CAN_GO_POS_X(this->_xPosInChunk) && CAN_GO_NEG_Y(this->_yPosInChunk)) {
		return *(_currentVoxel + POS_X_DELTA + NEG_Y_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume + 1, this->_yPosInVolume - 1, this->_zPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1ny1pz() const {
	if (CAN_GO_POS_X(this->_xPosInChunk) && CAN_GO_NEG_Y(this->_yPosInChunk) && CAN_GO_POS_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + POS_X_DELTA + NEG_Y_DELTA + POS_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume + 1, this->_yPosInVolume - 1, this->_zPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px0py1nz() const {
	if (CAN_GO_POS_X(this->_xPosInChunk) && CAN_GO_NEG_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + POS_X_DELTA + NEG_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume + 1, this->_yPosInVolume, this->_zPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px0py0pz() const {
	if (CAN_GO_POS_X(this->_xPosInChunk)) {
		return *(_currentVoxel + POS_X_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume + 1, this->_yPosInVolume, this->_zPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px0py1pz() const {
	if (CAN_GO_POS_X(this->_xPosInChunk) && CAN_GO_POS_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + POS_X_DELTA + POS_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume + 1, this->_yPosInVolume, this->_zPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1py1nz() const {
	if (CAN_GO_POS_X(this->_xPosInChunk) && CAN_GO_POS_Y(this->_yPosInChunk) && CAN_GO_NEG_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + POS_X_DELTA + POS_Y_DELTA + NEG_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume + 1, this->_yPosInVolume + 1, this->_zPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1py0pz() const {
	if (CAN_GO_POS_X(this->_xPosInChunk) && CAN_GO_POS_Y(this->_yPosInChunk)) {
		return *(_currentVoxel + POS_X_DELTA + POS_Y_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume + 1, this->_yPosInVolume + 1, this->_zPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1py1pz() const {
	if (CAN_GO_POS_X(this->_xPosInChunk) && CAN_GO_POS_Y(this->_yPosInChunk) && CAN_GO_POS_Z(this->_zPosInChunk)) {
		return *(_currentVoxel + POS_X_DELTA + POS_Y_DELTA + POS_Z_DELTA);
	}
	return this->_volume->voxel(this->_xPosInVolume + 1, this->_yPosInVolume + 1, this->_zPosInVolume + 1);
}

#undef CAN_GO_NEG_X
#undef CAN_GO_POS_X
#undef CAN_GO_NEG_Y
#undef CAN_GO_POS_Y
#undef CAN_GO_NEG_Z
#undef CAN_GO_POS_Z

#undef NEG_X_DELTA
#undef POS_X_DELTA
#undef NEG_Y_DELTA
#undef POS_Y_DELTA
#undef NEG_Z_DELTA
#undef POS_Z_DELTA

inline const Voxel& PagedVolume::BufferedSampler::voxel() const {
	return *_currentVoxel;
}

inline bool PagedVolume::BufferedSampler::setPosition(const glm::ivec3& v3dNewPos) {
	return setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
}

inline uint16_t PagedVolume::BufferedSampler::index(int x, int y, int z) const {
	core_assert_msg(x >= 0 && x < _regionWidth, "x: %i is out of bounds (0, %i)", x, _regionWidth);
	core_assert_msg(y >= 0 && y < _regionHeight, "y: %i is out of bounds (0, %i)", y, _regionHeight);
	core_assert_msg(z >= 0 && z < _regionDepth, "z: %i is out of bounds (0, %i)", z, _regionDepth);
	return z * _zOffset + y * _regionWidth + x;
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1nx1ny1nz() const {
	return _buffer[index(_xPosInBuffer - 1, _yPosInBuffer - 1, _zPosInBuffer - 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1nx1ny0pz() const {
	return _buffer[index(_xPosInBuffer - 1, _yPosInBuffer - 1, _zPosInBuffer)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1nx1ny1pz() const {
	return _buffer[index(_xPosInBuffer - 1, _yPosInBuffer - 1, _zPosInBuffer + 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1nx0py1nz() const {
	return _buffer[index(_xPosInBuffer - 1, _yPosInBuffer, _zPosInBuffer - 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1nx0py0pz() const {
	return _buffer[index(_xPosInBuffer - 1, _yPosInBuffer, _zPosInBuffer)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1nx0py1pz() const {
	return _buffer[index(_xPosInBuffer - 1, _yPosInBuffer, _zPosInBuffer + 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1nx1py1nz() const {
	return _buffer[index(_xPosInBuffer - 1, _yPosInBuffer + 1, _zPosInBuffer - 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1nx1py0pz() const {
	return _buffer[index(_xPosInBuffer - 1, _yPosInBuffer + 1, _zPosInBuffer)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1nx1py1pz() const {
	return _buffer[index(_xPosInBuffer - 1, _yPosInBuffer + 1, _zPosInBuffer + 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel0px1ny1nz() const {
	return _buffer[index(_xPosInBuffer, _yPosInBuffer - 1, _zPosInBuffer - 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel0px1ny0pz() const {
	return _buffer[index(_xPosInBuffer, _yPosInBuffer - 1, _zPosInBuffer)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel0px1ny1pz() const {
	return _buffer[index(_xPosInBuffer, _yPosInBuffer - 1, _zPosInBuffer + 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel0px0py1nz() const {
	return _buffer[index(_xPosInBuffer, _yPosInBuffer, _zPosInBuffer - 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel0px0py0pz() const {
	return *_currentVoxel;
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel0px0py1pz() const {
	return _buffer[index(_xPosInBuffer, _yPosInBuffer, _zPosInBuffer + 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel0px1py1nz() const {
	return _buffer[index(_xPosInBuffer, _yPosInBuffer + 1, _zPosInBuffer - 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel0px1py0pz() const {
	return _buffer[index(_xPosInBuffer, _yPosInBuffer + 1, _zPosInBuffer)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel0px1py1pz() const {
	return _buffer[index(_xPosInBuffer, _yPosInBuffer + 1, _zPosInBuffer + 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1px1ny1nz() const {
	return _buffer[index(_xPosInBuffer + 1, _yPosInBuffer - 1, _zPosInBuffer - 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1px1ny0pz() const {
	return _buffer[index(_xPosInBuffer + 1, _yPosInBuffer - 1, _zPosInBuffer)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1px1ny1pz() const {
	return _buffer[index(_xPosInBuffer + 1, _yPosInBuffer - 1, _zPosInBuffer + 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1px0py1nz() const {
	return _buffer[index(_xPosInBuffer + 1, _yPosInBuffer, _zPosInBuffer - 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1px0py0pz() const {
	return _buffer[index(_xPosInBuffer + 1, _yPosInBuffer, _zPosInBuffer)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1px0py1pz() const {
	return _buffer[index(_xPosInBuffer + 1, _yPosInBuffer, _zPosInBuffer + 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1px1py1nz() const {
	return _buffer[index(_xPosInBuffer + 1, _yPosInBuffer + 1, _zPosInBuffer - 1)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1px1py0pz() const {
	return _buffer[index(_xPosInBuffer + 1, _yPosInBuffer + 1, _zPosInBuffer)];
}

inline const Voxel& PagedVolume::BufferedSampler::peekVoxel1px1py1pz() const {
	return _buffer[index(_xPosInBuffer + 1, _yPosInBuffer + 1, _zPosInBuffer + 1)];
}

inline void PagedVolume::BufferedSampler::movePositiveX() {
	setPosition(_xPosInVolume + 1, _yPosInVolume, _zPosInVolume);
}

inline void PagedVolume::BufferedSampler::movePositiveY() {
	setPosition(_xPosInVolume, _yPosInVolume + 1, _zPosInVolume);
}

inline void PagedVolume::BufferedSampler::movePositiveZ() {
	setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume + 1);
}

inline void PagedVolume::BufferedSampler::moveNegativeX() {
	setPosition(_xPosInVolume - 1, _yPosInVolume, _zPosInVolume);
}

inline void PagedVolume::BufferedSampler::moveNegativeY() {
	setPosition(_xPosInVolume, _yPosInVolume - 1, _zPosInVolume);
}

inline void PagedVolume::BufferedSampler::moveNegativeZ() {
	setPosition(_xPosInVolume, _yPosInVolume, _zPosInVolume - 1);
}

inline bool PagedVolume::Chunk::containsPoint(const glm::ivec3& pos) const {
	 return region().containsPoint(pos);
}

inline bool PagedVolume::Chunk::containsPoint(int32_t x, int32_t y, int32_t z) const {
	 return region().containsPoint(x, y, z);
}

inline Region PagedVolume::Chunk::region() const {
	 const glm::ivec3 mins = _chunkSpacePosition * static_cast<int32_t>(_sideLength);
	 const glm::ivec3 maxs = mins + glm::ivec3(_sideLength - 1);
	 return Region(mins, maxs);
}

inline const Region& PagedVolume::region() const {
	return _region;
}

inline void PagedVolume::addChunkListener(IChunkListener* listener) {
	core::RecursiveScopedWriteLock writeLock(_listenerLock);
	_listener.push_back(listener);
}

inline void PagedVolume::removeChunkListener(IChunkListener* listener) {
	core::RecursiveScopedWriteLock writeLock(_listenerLock);
	for (auto i = _listener.begin(); i != _listener.end();) {
		if (listener == *i) {
			i = _listener.erase(i);
		} else {
			++i;
		}
	}
}

}
