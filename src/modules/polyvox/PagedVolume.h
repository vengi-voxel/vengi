#pragma once

#include "BaseVolume.h"
#include "Morton.h"

#include <array>
#include <algorithm>
#include <cstring> //For memcpy
#include <unordered_map>
#include <list>
#include <map>
#include <memory>
#include <vector>

namespace PolyVox {

/// This class provide a volume implementation which avoids storing all the data in memory at all times. Instead it breaks the volume
/// down into a set of chunks and moves these into and out of memory on demand. This means it is much more memory efficient than the
/// RawVolume, but may also be slower and is more complicated We encourage uses to work with RawVolume initially, and then switch to
/// PagedVolume once they have a larger application and/or a better understanding of PolyVox.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// The PagedVolume makes use of a Pager which defines the source and/or destination for data paged into and out of memory. PolyVox
/// comes with an example FilePager though users can also implement their own approaches. For example, the Pager could instead stream
/// data from a network connection or generate it procedurally on demand.
///
/// A consequence of this paging approach is that (unlike the RawVolume) the PagedVolume does not need to have a predefined size. After
/// the volume has been created you can begin acessing voxels anywhere in space and the required data will be created automatically.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
class PagedVolume: public BaseVolume<VoxelType> {
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

		VoxelType* getData() const;
		uint32_t getDataSizeInBytes() const;

		VoxelType getVoxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos) const;
		VoxelType getVoxel(const glm::i16vec3& v3dPos) const;

		void setVoxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos, VoxelType tValue);
		void setVoxel(const glm::i16vec3& v3dPos, VoxelType tValue);

		void changeLinearOrderingToMorton();
		void changeMortonOrderingToLinear();

	private:
		/// Private copy constructor to prevent accisdental copying
		Chunk(const Chunk& /*rhs*/);

		/// Private assignment operator to prevent accisdental copying
		Chunk& operator=(const Chunk& /*rhs*/);

		// This is updated by the PagedVolume and used to discard the least recently used chunks.
		uint32_t m_uChunkLastAccessed;

		// This is so we can tell whether a uncompressed chunk has to be recompressed and whether
		// a compressed chunk has to be paged back to disk, or whether they can just be discarded.
		bool m_bDataModified;

		uint32_t calculateSizeInBytes();
		static uint32_t calculateSizeInBytes(uint32_t uSideLength);

		VoxelType* m_tData;
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
	//option. For now it seems best to 'fix' it with the preprocessor insstead, but maybe the workaround can be reinstated
	//in the future
	//typedef Volume<VoxelType> VolumeOfVoxelType; //Workaround for GCC/VS2010 differences.
	//class Sampler : public VolumeOfVoxelType::template Sampler< PagedVolume<VoxelType> >
#if defined(_MSC_VER)
	class Sampler : public BaseVolume<VoxelType>::Sampler< PagedVolume<VoxelType> > //This line works on VS2010
#else
	class Sampler: public BaseVolume<VoxelType>::template Sampler<PagedVolume<VoxelType> > //This line works on GCC
#endif
	{
	public:
		Sampler(PagedVolume<VoxelType>* volume);
		~Sampler();

		inline VoxelType getVoxel() const;

		void setPosition(const glm::ivec3& v3dNewPos);
		void setPosition(int32_t xPos, int32_t yPos, int32_t zPos);
		inline bool setVoxel(VoxelType tValue);

		void movePositiveX();
		void movePositiveY();
		void movePositiveZ();

		void moveNegativeX();
		void moveNegativeY();
		void moveNegativeZ();

		inline VoxelType peekVoxel1nx1ny1nz() const;
		inline VoxelType peekVoxel1nx1ny0pz() const;
		inline VoxelType peekVoxel1nx1ny1pz() const;
		inline VoxelType peekVoxel1nx0py1nz() const;
		inline VoxelType peekVoxel1nx0py0pz() const;
		inline VoxelType peekVoxel1nx0py1pz() const;
		inline VoxelType peekVoxel1nx1py1nz() const;
		inline VoxelType peekVoxel1nx1py0pz() const;
		inline VoxelType peekVoxel1nx1py1pz() const;

		inline VoxelType peekVoxel0px1ny1nz() const;
		inline VoxelType peekVoxel0px1ny0pz() const;
		inline VoxelType peekVoxel0px1ny1pz() const;
		inline VoxelType peekVoxel0px0py1nz() const;
		inline VoxelType peekVoxel0px0py0pz() const;
		inline VoxelType peekVoxel0px0py1pz() const;
		inline VoxelType peekVoxel0px1py1nz() const;
		inline VoxelType peekVoxel0px1py0pz() const;
		inline VoxelType peekVoxel0px1py1pz() const;

		inline VoxelType peekVoxel1px1ny1nz() const;
		inline VoxelType peekVoxel1px1ny0pz() const;
		inline VoxelType peekVoxel1px1ny1pz() const;
		inline VoxelType peekVoxel1px0py1nz() const;
		inline VoxelType peekVoxel1px0py0pz() const;
		inline VoxelType peekVoxel1px0py1pz() const;
		inline VoxelType peekVoxel1px1py1nz() const;
		inline VoxelType peekVoxel1px1py0pz() const;
		inline VoxelType peekVoxel1px1py1pz() const;

	private:
		//Other current position information
		VoxelType* mCurrentVoxel;

		uint16_t m_uXPosInChunk;
		uint16_t m_uYPosInChunk;
		uint16_t m_uZPosInChunk;

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
	VoxelType getVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const;
	/// Gets a voxel at the position given by a 3D vector
	VoxelType getVoxel(const glm::ivec3& v3dPos) const;

	/// Sets the voxel at the position given by <tt>x,y,z</tt> coordinates
	void setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, VoxelType tValue);
	/// Sets the voxel at the position given by a 3D vector
	void setVoxel(const glm::ivec3& v3dPos, VoxelType tValue);

	/// Tries to ensure that the voxels within the specified Region are loaded into memory.
	void prefetch(Region regPrefetch);
	/// Removes all voxels from memory
	void flushAll();

	/// Calculates approximatly how many bytes of memory the volume is currently using.
	uint32_t calculateSizeInBytes();

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
};

////////////////////////////////////////////////////////////////////////////////
/// This constructor creates a volume with a fixed size which is specified as a parameter. By default this constructor will not enable paging but you can override this if desired. If you do wish to enable paging then you are required to provide the call back function (see the other PagedVolume constructor).
/// \param pPager Called by PolyVox to load and unload data on demand.
/// \param uTargetMemoryUsageInBytes The upper limit to how much memory this PagedVolume should aim to use.
/// \param uChunkSideLength The size of the chunks making up the volume. Small chunks will compress/decompress faster, but there will also be more of them meaning voxel access could be slower.
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
PagedVolume<VoxelType>::PagedVolume(Pager* pPager, uint32_t uTargetMemoryUsageInBytes, uint16_t uChunkSideLength) :
		BaseVolume<VoxelType>(), m_uChunkSideLength(uChunkSideLength), m_pPager(pPager) {
	// Validation of parameters
	core_assert_msg(pPager, "You must provide a valid pager when constructing a PagedVolume");
	core_assert_msg(uTargetMemoryUsageInBytes >= 1 * 1024 * 1024, "Target memory usage is too small to be practical");
	core_assert_msg(m_uChunkSideLength != 0, "Chunk side length cannot be zero.");
	core_assert_msg(m_uChunkSideLength <= 256, "Chunk size is too large to be practical.");
	core_assert_msg(isPowerOf2(m_uChunkSideLength), "Chunk side length must be a power of two.");

	// Used to perform multiplications and divisions by bit shifting.
	m_uChunkSideLengthPower = logBase2(m_uChunkSideLength);
	// Use to perform modulo by bit operations
	m_iChunkMask = m_uChunkSideLength - 1;

	// Calculate the number of chunks based on the memory limit and the size of each chunk.
	uint32_t uChunkSizeInBytes = PagedVolume<VoxelType>::Chunk::calculateSizeInBytes(m_uChunkSideLength);
	m_uChunkCountLimit = uTargetMemoryUsageInBytes / uChunkSizeInBytes;

	// Enforce sensible limits on the number of chunks.
	const uint32_t uMinPracticalNoOfChunks = 32; // Enough to make sure a chunks and it's neighbours can be loaded, with a few to spare.
	const uint32_t uMaxPracticalNoOfChunks = uChunkArraySize / 2; // A hash table should only become half-full to avoid too many clashes.
	if (m_uChunkCountLimit < uMinPracticalNoOfChunks) {
		::Log::warn("Requested memory usage limit of %uiMb is too low and cannot be adhered to.",
				uTargetMemoryUsageInBytes / (1024 * 1024));
	}
	m_uChunkCountLimit = std::max(m_uChunkCountLimit, uMinPracticalNoOfChunks);
	m_uChunkCountLimit = std::min(m_uChunkCountLimit, uMaxPracticalNoOfChunks);

	// Inform the user about the chosen memory configuration.
	::Log::debug("Memory usage limit for volume now set to %uiMb (%ui chunks of %uiKb each).",
			(m_uChunkCountLimit * uChunkSizeInBytes) / (1024 * 1024), m_uChunkCountLimit, uChunkSizeInBytes / 1024);
}

////////////////////////////////////////////////////////////////////////////////
/// Destroys the volume The destructor will call flushAll() to ensure that a paging volume has the chance to save it's data via the dataOverflowHandler() if desired.
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
PagedVolume<VoxelType>::~PagedVolume() {
	flushAll();
}

////////////////////////////////////////////////////////////////////////////////
/// This version of the function is provided so that the wrap mode does not need
/// to be specified as a template parameter, as it may be confusing to some users.
/// \param uXPos The \c x position of the voxel
/// \param uYPos The \c y position of the voxel
/// \param uZPos The \c z position of the voxel
/// \return The voxel value
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::getVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos) const {
	const int32_t chunkX = uXPos >> m_uChunkSideLengthPower;
	const int32_t chunkY = uYPos >> m_uChunkSideLengthPower;
	const int32_t chunkZ = uZPos >> m_uChunkSideLengthPower;

	const uint16_t xOffset = static_cast<uint16_t>(uXPos & m_iChunkMask);
	const uint16_t yOffset = static_cast<uint16_t>(uYPos & m_iChunkMask);
	const uint16_t zOffset = static_cast<uint16_t>(uZPos & m_iChunkMask);

	auto pChunk = canReuseLastAccessedChunk(chunkX, chunkY, chunkZ) ? m_pLastAccessedChunk : getChunk(chunkX, chunkY, chunkZ);

	return pChunk->getVoxel(xOffset, yOffset, zOffset);
}

////////////////////////////////////////////////////////////////////////////////
/// This version of the function is provided so that the wrap mode does not need
/// to be specified as a template parameter, as it may be confusing to some users.
/// \param v3dPos The 3D position of the voxel
/// \return The voxel value
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::getVoxel(const glm::ivec3& v3dPos) const {
	return getVoxel(v3dPos.x, v3dPos.y, v3dPos.z);
}

////////////////////////////////////////////////////////////////////////////////
/// \param uXPos the \c x position of the voxel
/// \param uYPos the \c y position of the voxel
/// \param uZPos the \c z position of the voxel
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
void PagedVolume<VoxelType>::setVoxel(int32_t uXPos, int32_t uYPos, int32_t uZPos, VoxelType tValue) {
	const int32_t chunkX = uXPos >> m_uChunkSideLengthPower;
	const int32_t chunkY = uYPos >> m_uChunkSideLengthPower;
	const int32_t chunkZ = uZPos >> m_uChunkSideLengthPower;

	const uint16_t xOffset = static_cast<uint16_t>(uXPos - (chunkX << m_uChunkSideLengthPower));
	const uint16_t yOffset = static_cast<uint16_t>(uYPos - (chunkY << m_uChunkSideLengthPower));
	const uint16_t zOffset = static_cast<uint16_t>(uZPos - (chunkZ << m_uChunkSideLengthPower));

	auto pChunk = canReuseLastAccessedChunk(chunkX, chunkY, chunkZ) ? m_pLastAccessedChunk : getChunk(chunkX, chunkY, chunkZ);

	pChunk->setVoxel(xOffset, yOffset, zOffset, tValue);
}

////////////////////////////////////////////////////////////////////////////////
/// \param v3dPos the 3D position of the voxel
/// \param tValue the value to which the voxel will be set
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
void PagedVolume<VoxelType>::setVoxel(const glm::ivec3& v3dPos, VoxelType tValue) {
	setVoxel(v3dPos.x, v3dPos.y, v3dPos.z, tValue);
}

////////////////////////////////////////////////////////////////////////////////
/// Note that if the memory usage limit is not large enough to support the region this function will only load part of the region. In this case it is undefined which parts will actually be loaded. If all the voxels in the given region are already loaded, this function will not do anything. Other voxels might be unloaded to make space for the new voxels.
/// \param regPrefetch The Region of voxels to prefetch into memory.
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
void PagedVolume<VoxelType>::prefetch(Region regPrefetch) {
	// Convert the start and end positions into chunk space coordinates
	const glm::ivec3& lower = regPrefetch.getLowerCorner();
	const glm::ivec3 v3dStart {lower.x >> m_uChunkSideLengthPower, lower.y >> m_uChunkSideLengthPower, lower.z >> m_uChunkSideLengthPower};

	const glm::ivec3& upper = regPrefetch.getUpperCorner();
	const glm::ivec3 v3dEnd {upper.x >> m_uChunkSideLengthPower, upper.y >> m_uChunkSideLengthPower, upper.z >> m_uChunkSideLengthPower};

	// Ensure we don't page in more chunks than the volume can hold.
	Region region(v3dStart, v3dEnd);
	uint32_t uNoOfChunks = static_cast<uint32_t>(region.getWidthInVoxels() * region.getHeightInVoxels() * region.getDepthInVoxels());
	if (uNoOfChunks > m_uChunkCountLimit) {
		::Log::warn("Attempting to prefetch more than the maximum number of chunks (this will cause thrashing).");
	}
	uNoOfChunks = std::min(uNoOfChunks, m_uChunkCountLimit);

	// Loops over the specified positions and touch the corresponding chunks.
	for (int32_t x = v3dStart.x; x <= v3dEnd.x; x++) {
		for (int32_t y = v3dStart.y; y <= v3dEnd.y; y++) {
			for (int32_t z = v3dStart.z; z <= v3dEnd.z; z++) {
				getChunk(x, y, z);
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////
/// Removes all voxels from memory, and calls dataOverflowHandler() to ensure the application has a chance to store the data.
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
void PagedVolume<VoxelType>::flushAll() {
	// Clear this pointer as all chunks are about to be removed.
	m_pLastAccessedChunk = nullptr;

	// Erase all the most recently used chunks.
	for (uint32_t uIndex = 0; uIndex < uChunkArraySize; uIndex++) {
		m_arrayChunks[uIndex] = nullptr;
	}
}

template<typename VoxelType>
bool PagedVolume<VoxelType>::canReuseLastAccessedChunk(int32_t iChunkX, int32_t iChunkY, int32_t iChunkZ) const {
	return iChunkX == m_v3dLastAccessedChunkX && iChunkY == m_v3dLastAccessedChunkY && iChunkZ == m_v3dLastAccessedChunkZ && m_pLastAccessedChunk;
}

template<typename VoxelType>
typename PagedVolume<VoxelType>::Chunk* PagedVolume<VoxelType>::getChunk(int32_t uChunkX, int32_t uChunkY, int32_t uChunkZ) const {
	Chunk* pChunk = nullptr;

	// We generate a 16-bit hash here and assume this matches the range available in the chunk
	// array. The assert here is just to make sure we take care if change this in the future.
	static_assert(uChunkArraySize == 65536, "Chunk array size has changed, check if the hash calculation needs updating.");
	// Extract the lower five bits from each position component.
	const uint32_t uChunkXLowerBits = static_cast<uint32_t>(uChunkX & 0x1F);
	const uint32_t uChunkYLowerBits = static_cast<uint32_t>(uChunkY & 0x1F);
	const uint32_t uChunkZLowerBits = static_cast<uint32_t>(uChunkZ & 0x1F);
	// Combine then to form a 15-bit hash of the position. Also shift by one to spread the values out in the whole 16-bit space.
	const uint32_t iPosisionHash = (((uChunkXLowerBits)) | ((uChunkYLowerBits) << 5) | ((uChunkZLowerBits) << 10) << 1);

	// Starting at the position indicated by the hash, and then search through the whole array looking for a chunk with the correct
	// position. In most cases we expect to find it in the first place we look. Note that this algorithm is slow in the case that
	// the chunk is not found because the whole array has to be searched, but in this case we are going to have to page the data in
	// from an external source which is likely to be slow anyway.
	uint32_t iIndex = iPosisionHash;
	do {
		if (m_arrayChunks[iIndex]) {
			glm::ivec3& entryPos = m_arrayChunks[iIndex]->m_v3dChunkSpacePosition;
			if (entryPos.x == uChunkX && entryPos.y == uChunkY && entryPos.z == uChunkZ) {
				pChunk = m_arrayChunks[iIndex].get();
				pChunk->m_uChunkLastAccessed = ++m_uTimestamper;
				break;
			}
		}

		iIndex++;
		iIndex %= uChunkArraySize;
	} while (iIndex != iPosisionHash); // Keep searching until we get back to our start position.

	// If we still haven't found the chunk then it's time to create a new one and page it in from disk.
	if (!pChunk) {
		// The chunk was not found so we will create a new one.
		glm::ivec3 v3dChunkPos(uChunkX, uChunkY, uChunkZ);
		pChunk = new PagedVolume<VoxelType>::Chunk(v3dChunkPos, m_uChunkSideLength, m_pPager);
		pChunk->m_uChunkLastAccessed = ++m_uTimestamper; // Important, as we may soon delete the oldest chunk

		// Store the chunk at the appropriate place in out chunk array. Ideally this place is
		// given by the hash, otherwise we do a linear search for the next available location
		// We always expect to find a free place because we aim to keep the array only half full.
		uint32_t iIndex = iPosisionHash;
		bool bInsertedSucessfully = false;
		do {
			if (m_arrayChunks[iIndex] == nullptr) {
				m_arrayChunks[iIndex] = std::move(std::unique_ptr<Chunk>(pChunk));
				bInsertedSucessfully = true;
				break;
			}

			iIndex++;
			iIndex %= uChunkArraySize;
		} while (iIndex != iPosisionHash); // Keep searching until we get back to our start position.

		// This should never really happen unless we are failing to keep our number of active chunks
		// significantly under the target amount. Perhaps if chunks are 'pinned' for threading purposes?
		core_assert_msg(bInsertedSucessfully, "No space in chunk array for new chunk.");

		// As we have added a chunk we may have exceeded our target chunk limit. Search through the array to
		// determine how many chunks we have, as well as finding the oldest timestamp. Note that this is potentially
		// wasteful and we may instead wish to track how many chunks we have and/or delete a chunk at random (or
		// just check e.g. 10 and delete the oldest of those) but we'll see if this is a bottleneck first. Paging
		// the data in is probably more expensive.
		uint32_t uChunkCount = 0;
		uint32_t uOldestChunkIndex = 0;
		uint32_t uOldestChunkTimestamp = std::numeric_limits<uint32_t>::max();
		for (uint32_t uIndex = 0; uIndex < uChunkArraySize; uIndex++) {
			if (m_arrayChunks[uIndex]) {
				uChunkCount++;
				if (m_arrayChunks[uIndex]->m_uChunkLastAccessed < uOldestChunkTimestamp) {
					uOldestChunkTimestamp = m_arrayChunks[uIndex]->m_uChunkLastAccessed;
					uOldestChunkIndex = uIndex;
				}
			}
		}

		// Check if we have too many chunks, and delete the oldest if so.
		if (uChunkCount > m_uChunkCountLimit) {
			m_arrayChunks[uOldestChunkIndex] = nullptr;
		}
	}

	m_pLastAccessedChunk = pChunk;
	m_v3dLastAccessedChunkX = uChunkX;
	m_v3dLastAccessedChunkY = uChunkY;
	m_v3dLastAccessedChunkZ = uChunkZ;

	return pChunk;
}

////////////////////////////////////////////////////////////////////////////////
/// Calculate the memory usage of the volume.
////////////////////////////////////////////////////////////////////////////////
template<typename VoxelType>
uint32_t PagedVolume<VoxelType>::calculateSizeInBytes() {
	uint32_t uChunkCount = 0;
	for (uint32_t uIndex = 0; uIndex < uChunkArraySize; uIndex++) {
		if (m_arrayChunks[uIndex]) {
			uChunkCount++;
		}
	}

	// Note: We disregard the size of the other class members as they are likely to be very small compared to the size of the
	// allocated voxel data. This also keeps the reported size as a power of two, which makes other memory calculations easier.
	return PagedVolume<VoxelType>::Chunk::calculateSizeInBytes(m_uChunkSideLength) * uChunkCount;
}

template<typename VoxelType>
PagedVolume<VoxelType>::Chunk::Chunk(glm::ivec3 v3dPosition, uint16_t uSideLength, Pager* pPager) :
		m_uChunkLastAccessed(0), m_bDataModified(true), m_tData(0), m_uSideLength(0), m_uSideLengthPower(0), m_pPager(pPager), m_v3dChunkSpacePosition(v3dPosition) {
	core_assert_msg(m_pPager, "No valid pager supplied to chunk constructor.");
	core_assert_msg(uSideLength <= 256, "Chunk side length cannot be greater than 256.");

	// Compute the side length
	m_uSideLength = uSideLength;
	m_uSideLengthPower = logBase2(uSideLength);

	// Allocate the data
	const uint32_t uNoOfVoxels = m_uSideLength * m_uSideLength * m_uSideLength;
	m_tData = new VoxelType[uNoOfVoxels];

	// Pass the chunk to the Pager to give it a chance to initialise it with any data
	// From the coordinates of the chunk we deduce the coordinates of the contained voxels.
	const glm::ivec3 v3dLower = m_v3dChunkSpacePosition * static_cast<int32_t>(m_uSideLength);
	const glm::ivec3 v3dUpper = v3dLower + glm::ivec3(m_uSideLength - 1, m_uSideLength - 1, m_uSideLength - 1);
	const Region reg(v3dLower, v3dUpper);

	// A valid pager is normally present - this check is mostly to ease unit testing.
	if (m_pPager) {
		// Page the data in
		m_pPager->pageIn(reg, this);
	}

	// We'll use this later to decide if data needs to be paged out again.
	m_bDataModified = false;
}

template<typename VoxelType>
PagedVolume<VoxelType>::Chunk::~Chunk() {
	if (m_bDataModified && m_pPager) {
		// From the coordinates of the chunk we deduce the coordinates of the contained voxels.
		const glm::ivec3 v3dLower = m_v3dChunkSpacePosition * static_cast<int32_t>(m_uSideLength);
		const glm::ivec3 v3dUpper = v3dLower + glm::ivec3(m_uSideLength - 1, m_uSideLength - 1, m_uSideLength - 1);

		// Page the data out
		m_pPager->pageOut(Region(v3dLower, v3dUpper), this);
	}

	delete[] m_tData;
	m_tData = 0;
}

template<typename VoxelType>
VoxelType* PagedVolume<VoxelType>::Chunk::getData() const {
	return m_tData;
}

template<typename VoxelType>
uint32_t PagedVolume<VoxelType>::Chunk::getDataSizeInBytes() const {
	return m_uSideLength * m_uSideLength * m_uSideLength * sizeof(VoxelType);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Chunk::getVoxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos) const {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path so we use asserts rather than exceptions.
	core_assert_msg(uXPos < m_uSideLength, "Supplied position is outside of the chunk");
	core_assert_msg(uYPos < m_uSideLength, "Supplied position is outside of the chunk");
	core_assert_msg(uZPos < m_uSideLength, "Supplied position is outside of the chunk");
	core_assert_msg(m_tData, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	const uint32_t index = morton256_x[uXPos] | morton256_y[uYPos] | morton256_z[uZPos];
	return m_tData[index];
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Chunk::getVoxel(const glm::i16vec3& v3dPos) const {
	return getVoxel(v3dPos.x, v3dPos.y, v3dPos.z);
}

template<typename VoxelType>
void PagedVolume<VoxelType>::Chunk::setVoxel(uint32_t uXPos, uint32_t uYPos, uint32_t uZPos, VoxelType tValue) {
	// This code is not usually expected to be called by the user, with the exception of when implementing paging
	// of uncompressed data. It's a performance critical code path so we use asserts rather than exceptions.
	core_assert_msg(uXPos < m_uSideLength, "Supplied position is outside of the chunk");
	core_assert_msg(uYPos < m_uSideLength, "Supplied position is outside of the chunk");
	core_assert_msg(uZPos < m_uSideLength, "Supplied position is outside of the chunk");
	core_assert_msg(m_tData, "No uncompressed data - chunk must be decompressed before accessing voxels.");

	const uint32_t index = morton256_x[uXPos] | morton256_y[uYPos] | morton256_z[uZPos];
	m_tData[index] = tValue;
	this->m_bDataModified = true;
}

template<typename VoxelType>
void PagedVolume<VoxelType>::Chunk::setVoxel(const glm::i16vec3& v3dPos, VoxelType tValue) {
	setVoxel(v3dPos.x, v3dPos.y, v3dPos.z, tValue);
}

template<typename VoxelType>
uint32_t PagedVolume<VoxelType>::Chunk::calculateSizeInBytes() {
	// Call through to the static version
	return calculateSizeInBytes(m_uSideLength);
}

template<typename VoxelType>
uint32_t PagedVolume<VoxelType>::Chunk::calculateSizeInBytes(uint32_t uSideLength) {
	// Note: We disregard the size of the other class members as they are likely to be very small compared to the size of the
	// allocated voxel data. This also keeps the reported size as a power of two, which makes other memory calculations easier.
	uint32_t uSizeInBytes = uSideLength * uSideLength * uSideLength * sizeof(VoxelType);
	return uSizeInBytes;
}

// This convienience function exists for historical reasons. Chunks used to store their data in 'linear' order but now we
// use Morton encoding. Users who still have data in linear order (on disk, in databases, etc) will need to call this function
// if they load the data in by memcpy()ing it via the raw pointer. On the other hand, if they set the data using setVoxel()
// then the ordering is automatically handled correctly.
template<typename VoxelType>
void PagedVolume<VoxelType>::Chunk::changeLinearOrderingToMorton() {
	VoxelType* pTempBuffer = new VoxelType[m_uSideLength * m_uSideLength * m_uSideLength];

	// We should prehaps restructure this loop. From: https://fgiesen.wordpress.com/2011/01/17/texture-tiling-and-swizzling/
	//
	// "There's two basic ways to structure the actual swizzling: either you go through the (linear) source image in linear order,
	// writing in (somewhat) random order, or you iterate over the output data, picking the right source pixel for each target
	// location. The former is more natural, especially when updating subrects of the destination texture (the source pixels still
	// consist of one linear sequence of bytes per line; the pattern of destination addresses written is considerably more
	// complicated), but the latter is usually much faster, especially if the source image data is in cached memory while the output
	// data resides in non-cached write-combined memory where non-sequential writes are expensive."
	//
	// This is something to consider if profiling identifies it as a hotspot.
	for (uint16_t z = 0; z < m_uSideLength; z++) {
		for (uint16_t y = 0; y < m_uSideLength; y++) {
			for (uint16_t x = 0; x < m_uSideLength; x++) {
				uint32_t uLinearIndex = x + y * m_uSideLength + z * m_uSideLength * m_uSideLength;
				uint32_t uMortonIndex = morton256_x[x] | morton256_y[y] | morton256_z[z];
				pTempBuffer[uMortonIndex] = m_tData[uLinearIndex];
			}
		}
	}

	std::memcpy(m_tData, pTempBuffer, getDataSizeInBytes());

	delete[] pTempBuffer;
}

// Like the above function, this is provided fot easing backwards compatibility. In Cubiquity we have some
// old databases which use linear ordering, and we need to continue to save such data in linear order.
template<typename VoxelType>
void PagedVolume<VoxelType>::Chunk::changeMortonOrderingToLinear() {
	VoxelType* pTempBuffer = new VoxelType[m_uSideLength * m_uSideLength * m_uSideLength];
	for (uint16_t z = 0; z < m_uSideLength; z++) {
		for (uint16_t y = 0; y < m_uSideLength; y++) {
			for (uint16_t x = 0; x < m_uSideLength; x++) {
				uint32_t uLinearIndex = x + y * m_uSideLength + z * m_uSideLength * m_uSideLength;
				uint32_t uMortonIndex = morton256_x[x] | morton256_y[y] | morton256_z[z];
				pTempBuffer[uLinearIndex] = m_tData[uMortonIndex];
			}
		}
	}

	std::memcpy(m_tData, pTempBuffer, getDataSizeInBytes());

	delete[] pTempBuffer;
}

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

template<typename VoxelType>
PagedVolume<VoxelType>::Sampler::Sampler(PagedVolume<VoxelType>* volume) :
		BaseVolume<VoxelType>::template Sampler<PagedVolume<VoxelType> >(volume), m_uChunkSideLengthMinusOne(volume->m_uChunkSideLength - 1) {
}

template<typename VoxelType>
PagedVolume<VoxelType>::Sampler::~Sampler() {
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::getVoxel() const {
	return *mCurrentVoxel;
}

template<typename VoxelType>
void PagedVolume<VoxelType>::Sampler::setPosition(const glm::ivec3& v3dNewPos) {
	setPosition(v3dNewPos.x, v3dNewPos.y, v3dNewPos.z);
}

template<typename VoxelType>
void PagedVolume<VoxelType>::Sampler::setPosition(int32_t xPos, int32_t yPos, int32_t zPos) {
	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<PagedVolume<VoxelType> >::setPosition(xPos, yPos, zPos);

	// Then we update the voxel pointer
	const int32_t uXChunk = this->mXPosInVolume >> this->mVolume->m_uChunkSideLengthPower;
	const int32_t uYChunk = this->mYPosInVolume >> this->mVolume->m_uChunkSideLengthPower;
	const int32_t uZChunk = this->mZPosInVolume >> this->mVolume->m_uChunkSideLengthPower;

	m_uXPosInChunk = static_cast<uint16_t>(this->mXPosInVolume - (uXChunk << this->mVolume->m_uChunkSideLengthPower));
	m_uYPosInChunk = static_cast<uint16_t>(this->mYPosInVolume - (uYChunk << this->mVolume->m_uChunkSideLengthPower));
	m_uZPosInChunk = static_cast<uint16_t>(this->mZPosInVolume - (uZChunk << this->mVolume->m_uChunkSideLengthPower));

	uint32_t uVoxelIndexInChunk = morton256_x[m_uXPosInChunk] | morton256_y[m_uYPosInChunk] | morton256_z[m_uZPosInChunk];

	auto pCurrentChunk =
			this->mVolume->canReuseLastAccessedChunk(uXChunk, uYChunk, uZChunk) ? this->mVolume->m_pLastAccessedChunk : this->mVolume->getChunk(uXChunk, uYChunk, uZChunk);

	mCurrentVoxel = pCurrentChunk->m_tData + uVoxelIndexInChunk;
}

template<typename VoxelType>
bool PagedVolume<VoxelType>::Sampler::setVoxel(VoxelType tValue) {
	//Need to think what effect this has on any existing iterators.
	core_assert_msg(false, "This function cannot be used on PagedVolume samplers.");
	return false;
}

template<typename VoxelType>
void PagedVolume<VoxelType>::Sampler::movePositiveX() {
	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<PagedVolume<VoxelType> >::movePositiveX();

	// Then we update the voxel pointer
	if (CAN_GO_POS_X(this->m_uXPosInChunk)) {
		//No need to compute new chunk.
		mCurrentVoxel += POS_X_DELTA;
		this->m_uXPosInChunk++;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
void PagedVolume<VoxelType>::Sampler::movePositiveY() {
	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<PagedVolume<VoxelType> >::movePositiveY();

	// Then we update the voxel pointer
	if (CAN_GO_POS_Y(this->m_uYPosInChunk)) {
		//No need to compute new chunk.
		mCurrentVoxel += POS_Y_DELTA;
		this->m_uYPosInChunk++;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
void PagedVolume<VoxelType>::Sampler::movePositiveZ() {
	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<PagedVolume<VoxelType> >::movePositiveZ();

	// Then we update the voxel pointer
	if (CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		//No need to compute new chunk.
		mCurrentVoxel += POS_Z_DELTA;
		this->m_uZPosInChunk++;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
void PagedVolume<VoxelType>::Sampler::moveNegativeX() {
	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<PagedVolume<VoxelType> >::moveNegativeX();

	// Then we update the voxel pointer
	if (CAN_GO_NEG_X(this->m_uXPosInChunk)) {
		//No need to compute new chunk.
		mCurrentVoxel += NEG_X_DELTA;
		this->m_uXPosInChunk--;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
void PagedVolume<VoxelType>::Sampler::moveNegativeY() {
	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<PagedVolume<VoxelType> >::moveNegativeY();

	// Then we update the voxel pointer
	if (CAN_GO_NEG_Y(this->m_uYPosInChunk)) {
		//No need to compute new chunk.
		mCurrentVoxel += NEG_Y_DELTA;
		this->m_uYPosInChunk--;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
void PagedVolume<VoxelType>::Sampler::moveNegativeZ() {
	// Base version updates position and validity flags.
	BaseVolume<VoxelType>::template Sampler<PagedVolume<VoxelType> >::moveNegativeZ();

	// Then we update the voxel pointer
	if (CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		//No need to compute new chunk.
		mCurrentVoxel += NEG_Z_DELTA;
		this->m_uZPosInChunk--;
	} else {
		//We've hit the chunk boundary. Just calling setPosition() is the easiest way to resolve this.
		setPosition(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume);
	}
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1nx1ny1nz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + NEG_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume - 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1nx1ny0pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + NEG_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume - 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1nx1ny1pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + NEG_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume - 1, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1nx0py1nz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1nx0py0pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1nx0py1pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1nx1py1nz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + POS_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume + 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1nx1py0pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + POS_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume + 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1nx1py1pz() const {
	if (CAN_GO_NEG_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_X_DELTA + POS_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume - 1, this->mYPosInVolume + 1, this->mZPosInVolume + 1);
}

//////////////////////////////////////////////////////////////////////////

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel0px1ny1nz() const {
	if (CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume - 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel0px1ny0pz() const {
	if (CAN_GO_NEG_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + NEG_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume - 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel0px1ny1pz() const {
	if (CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume - 1, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel0px0py1nz() const {
	if (CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel0px0py0pz() const {
	return *mCurrentVoxel;
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel0px0py1pz() const {
	if (CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel0px1py1nz() const {
	if (CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume + 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel0px1py0pz() const {
	if (CAN_GO_POS_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + POS_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume + 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel0px1py1pz() const {
	if (CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume, this->mYPosInVolume + 1, this->mZPosInVolume + 1);
}

//////////////////////////////////////////////////////////////////////////

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1px1ny1nz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + NEG_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume - 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1px1ny0pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + NEG_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume - 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1px1ny1pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_NEG_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + NEG_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume - 1, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1px0py1nz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1px0py0pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1px0py1pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume, this->mZPosInVolume + 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1px1py1nz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_NEG_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + POS_Y_DELTA + NEG_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume + 1, this->mZPosInVolume - 1);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1px1py0pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + POS_Y_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume + 1, this->mZPosInVolume);
}

template<typename VoxelType>
VoxelType PagedVolume<VoxelType>::Sampler::peekVoxel1px1py1pz() const {
	if (CAN_GO_POS_X(this->m_uXPosInChunk) && CAN_GO_POS_Y(this->m_uYPosInChunk) && CAN_GO_POS_Z(this->m_uZPosInChunk)) {
		return *(mCurrentVoxel + POS_X_DELTA + POS_Y_DELTA + POS_Z_DELTA);
	}
	return this->mVolume->getVoxel(this->mXPosInVolume + 1, this->mYPosInVolume + 1, this->mZPosInVolume + 1);
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
