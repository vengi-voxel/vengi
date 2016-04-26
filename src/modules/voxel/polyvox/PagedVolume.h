#pragma once

#include "BaseVolume.h"
#include "Morton.h"
#include "Voxel.h"
#include "core/ReadWriteLock.h"
#include <array>
#include <algorithm>
#include <cstring> //For memcpy
#include <unordered_map>
#include <list>
#include <map>
#include <memory>
#include <vector>

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
class PagedVolume: public BaseVolume {
public:
	/// The PagedVolume stores it data as a set of Chunk instances which can be loaded and unloaded as memory requirements dictate.
	class Chunk;
	/// The Pager class is responsible for the loading and unloading of Chunks, and can be subclassed by the user.
	class Pager;

	class Chunk {
		friend class PagedVolume;

	public:
		Chunk(glm::ivec3 v3dPosition, uint16_t uSideLength, Pager* pPager = nullptr);
		~Chunk();

		Voxel* getData() const;
		uint32_t getDataSizeInBytes() const;

		const Voxel& getVoxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos) const;
		const Voxel& getVoxel(const glm::i16vec3& v3dPos) const;

		void setVoxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos, Voxel tValue);
		void setVoxel(const glm::i16vec3& v3dPos, Voxel tValue);

	private:
		// This is updated by the PagedVolume and used to discard the least recently used chunks.
		uint32_t m_uChunkLastAccessed;

		// This is so we can tell whether a uncompressed chunk has to be recompressed and whether
		// a compressed chunk has to be paged back to disk, or whether they can just be discarded.
		bool m_bDataModified;

		uint32_t calculateSizeInBytes();
		static uint32_t calculateSizeInBytes(uint32_t uSideLength);

		Voxel* m_tData;
		uint16_t m_uSideLength;
		uint8_t m_uSideLengthPower;
		Pager* m_pPager;

		// Note: Do we really need to store this position here as well as in the block maps?
		glm::ivec3 m_v3dChunkSpacePosition;
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

		virtual void pageIn(const Region& region, Chunk* pChunk) = 0;
		virtual void pageOut(const Region& region, Chunk* pChunk) = 0;
	};

	//There seems to be some descrepency between Visual Studio and GCC about how the following class should be declared.
	//There is a work around (see also See http://goo.gl/qu1wn) given below which appears to work on VS2010 and GCC, but
	//which seems to cause internal compiler errors on VS2008 when building with the /Gm 'Enable Minimal Rebuild' compiler
	//option. For now it seems best to 'fix' it with the preprocessor instead, but maybe the workaround can be reinstated
	//in the future
	//typedef Volume<Voxel> VolumeOfVoxelType; //Workaround for GCC/VS2010 differences.
	//class Sampler : public VolumeOfVoxelType::template Sampler< PagedVolume >
#if defined(_MSC_VER)
	class Sampler : public BaseVolume::Sampler< PagedVolume > //This line works on VS2010
#else
	class Sampler: public BaseVolume::template Sampler<PagedVolume > //This line works on GCC
#endif
	{
	public:
		Sampler(PagedVolume* volume);
		~Sampler();

		inline const Voxel& getVoxel() const;

		inline void setPosition(const glm::ivec3& v3dNewPos);
		void setPosition(int32_t xPos, int32_t yPos, int32_t zPos);
		inline bool setVoxel(const Voxel& tValue);

		void movePositiveX();
		void movePositiveY();
		void movePositiveZ();

		void moveNegativeX();
		void moveNegativeY();
		void moveNegativeZ();

		inline const Voxel& peekVoxel1nx1ny1nz() const;
		inline const Voxel& peekVoxel1nx1ny0pz() const;
		inline const Voxel& peekVoxel1nx1ny1pz() const;
		inline const Voxel& peekVoxel1nx0py1nz() const;
		inline const Voxel& peekVoxel1nx0py0pz() const;
		inline const Voxel& peekVoxel1nx0py1pz() const;
		inline const Voxel& peekVoxel1nx1py1nz() const;
		inline const Voxel& peekVoxel1nx1py0pz() const;
		inline const Voxel& peekVoxel1nx1py1pz() const;

		inline const Voxel& peekVoxel0px1ny1nz() const;
		inline const Voxel& peekVoxel0px1ny0pz() const;
		inline const Voxel& peekVoxel0px1ny1pz() const;
		inline const Voxel& peekVoxel0px0py1nz() const;
		inline const Voxel& peekVoxel0px0py0pz() const;
		inline const Voxel& peekVoxel0px0py1pz() const;
		inline const Voxel& peekVoxel0px1py1nz() const;
		inline const Voxel& peekVoxel0px1py0pz() const;
		inline const Voxel& peekVoxel0px1py1pz() const;

		inline const Voxel& peekVoxel1px1ny1nz() const;
		inline const Voxel& peekVoxel1px1ny0pz() const;
		inline const Voxel& peekVoxel1px1ny1pz() const;
		inline const Voxel& peekVoxel1px0py1nz() const;
		inline const Voxel& peekVoxel1px0py0pz() const;
		inline const Voxel& peekVoxel1px0py1pz() const;
		inline const Voxel& peekVoxel1px1py1nz() const;
		inline const Voxel& peekVoxel1px1py0pz() const;
		inline const Voxel& peekVoxel1px1py1pz() const;

	private:
		//Other current position information
		Voxel* mCurrentVoxel = nullptr;

		uint16_t m_uXPosInChunk = 0u;
		uint16_t m_uYPosInChunk = 0u;
		uint16_t m_uZPosInChunk = 0u;

		// This should ideally be const, but that prevent automatic generation of an assignment operator (https://goo.gl/Sn7KpZ).
		// We could provide one manually, but it's currently unused so there is no real test for if it works. I'm putting
		// together a new release at the moment so I'd rathern not make 'risky' changes.
		uint16_t m_uChunkSideLengthMinusOne;
	};

public:
	/// Constructor for creating a fixed size volume.
	PagedVolume(Pager* pPager, uint32_t uTargetMemoryUsageInBytes = 256 * 1024 * 1024, uint16_t uChunkSideLength = 32);
	/// Destructor
	~PagedVolume();

	/// Gets a voxel at the position given by <tt>x,y,z</tt> coordinates
	const Voxel& getVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const;
	/// Gets a voxel at the position given by a 3D vector
	const Voxel& getVoxel(const glm::ivec3& v3dPos) const;

	/// Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	void setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, const Voxel& tValue);
	/// Sets the voxel at the position given by a 3D vector
	void setVoxel(const glm::ivec3& v3dPos, const Voxel& tValue);
	/// Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	void setVoxels(int32_t uXPos, int32_t uZPos, const Voxel* tArray, int amount);

	/// Tries to ensure that the voxels within the specified Region are loaded into memory.
	void prefetch(Region regPrefetch);
	/// Removes all voxels from memory
	void flushAll();

	/// Calculates approximatly how many bytes of memory the volume is currently using.
	uint32_t calculateSizeInBytes();
	Chunk* getChunk(const glm::ivec3& pos) const;

protected:
	/// Copy constructor
	PagedVolume(const PagedVolume& rhs);

	/// Assignment operator
	PagedVolume& operator=(const PagedVolume& rhs);

private:
	bool canReuseLastAccessedChunk(int32_t iChunkX, int32_t iChunkY, int32_t iChunkZ) const;
	Chunk* getChunk(int32_t uChunkX, int32_t uChunkY, int32_t uChunkZ) const;

	// Storing these properties individually has proved to be faster than keeping
	// them in a glm::ivec3 as it avoids constructions and comparison overheads.
	// They are also at the start of the class in the hope that they will be pulled
	// into cache - I've got no idea if this actually makes a difference.
	mutable int32_t m_v3dLastAccessedChunkX = 0;
	mutable int32_t m_v3dLastAccessedChunkY = 0;
	mutable int32_t m_v3dLastAccessedChunkZ = 0;
	mutable Chunk* m_pLastAccessedChunk = nullptr;

	mutable uint32_t m_uTimestamper = 0;

	uint32_t m_uChunkCountLimit = 0;

	// Chunks are stored in the following array which is used as a hash-table. Conventional wisdom is that such a hash-table
	// should not be more than half full to avoid conflicts, and a practical chunk size seems to be 64^3. With this configuration
	// there can be up to 32768*64^3 = 8 gigavoxels (with each voxel perhaps being many bytes). This should effectively make use
	// of even high end machines. Of course, the user can choose to limit the memory usage in which case much less of the chunk
	// array will actually be used. None-the-less, we have chosen to use a fixed size array (rather than a vector) as it appears to
	// be slightly faster (probably due to the extra pointer indirection in a vector?) and the actual size of this array should
	// just be 1Mb or so.
	static const uint32_t uChunkArraySize = 65536;
	mutable std::unique_ptr<Chunk> m_arrayChunks[uChunkArraySize];

	// The size of the chunks
	uint16_t m_uChunkSideLength;
	uint8_t m_uChunkSideLengthPower;
	int32_t m_iChunkMask;

	Pager* m_pPager = nullptr;

	mutable core::ReadWriteLock _lock;
};

inline const Voxel& PagedVolume::Sampler::getVoxel() const {
	return *mCurrentVoxel;
}

inline void PagedVolume::Sampler::setPosition(const glm::ivec3& v3dNewPos) {
	setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
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
#define CAN_GO_POS_X(val)  (val < this->m_uChunkSideLengthMinusOne)
#define CAN_GO_NEG_Y(val) (val > 0)
#define CAN_GO_POS_Y(val)  (val < this->m_uChunkSideLengthMinusOne)
#define CAN_GO_NEG_Z(val) (val > 0)
#define CAN_GO_POS_Z(val)  (val < this->m_uChunkSideLengthMinusOne)

#define NEG_X_DELTA (-(deltaX[this->m_uXPosInChunk-1]))
#define POS_X_DELTA (deltaX[this->m_uXPosInChunk])
#define NEG_Y_DELTA (-(deltaY[this->m_uYPosInChunk-1]))
#define POS_Y_DELTA (deltaY[this->m_uYPosInChunk])
#define NEG_Z_DELTA (-(deltaZ[this->m_uZPosInChunk-1]))
#define POS_Z_DELTA (deltaZ[this->m_uZPosInChunk])

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1ny1nz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + NEG_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume - 1, this->mZPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1ny0pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + NEG_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume - 1, this->mZPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1ny1pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + NEG_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume - 1, this->mZPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx0py1nz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume, this->mZPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx0py0pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume, this->mZPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx0py1pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume, this->mZPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1py1nz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + POS_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume + 1, this->mZPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1py0pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + POS_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume + 1, this->mZPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1nx1py1pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + POS_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume + 1, this->mZPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1ny1nz() const {
	if (CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume - 1, this->mZPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1ny0pz() const {
	if (CAN_GO_NEG_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + NEG_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume - 1, this->mZPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1ny1pz() const {
	if (CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume - 1, this->mZPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px0py1nz() const {
	if (CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px0py0pz() const {
	return *mCurrentVoxel;
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px0py1pz() const {
	if (CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1py1nz() const {
	if (CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume + 1, this->mZPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1py0pz() const {
	if (CAN_GO_POS_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + POS_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume + 1, this->mZPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel0px1py1pz() const {
	if (CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume + 1, this->mZPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1ny1nz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + NEG_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume - 1, this->mZPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1ny0pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + NEG_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume - 1, this->mZPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1ny1pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + NEG_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume - 1, this->mZPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px0py1nz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume, this->mZPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px0py0pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume, this->mZPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px0py1pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume, this->mZPosInVolume + 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1py1nz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + POS_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume + 1, this->mZPosInVolume - 1);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1py0pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + POS_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume + 1, this->mZPosInVolume);
}

inline const Voxel& PagedVolume::Sampler::peekVoxel1px1py1pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + POS_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume + 1, this->mZPosInVolume + 1);
}

inline bool PagedVolume::canReuseLastAccessedChunk(int32_t iChunkX, int32_t iChunkY, int32_t iChunkZ) const {
	return iChunkX == m_v3dLastAccessedChunkX && iChunkY == m_v3dLastAccessedChunkY && iChunkZ == m_v3dLastAccessedChunkZ && m_pLastAccessedChunk;
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



}
