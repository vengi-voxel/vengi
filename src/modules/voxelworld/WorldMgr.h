/**
 * @file
 * @defgroup Voxel
 * @{
 * @}
 */

#pragma once

#include "voxel/Mesh.h"
#include "voxel/PagedVolume.h"
#include "voxelutil/Raycast.h"
#include "voxelformat/VolumeCache.h"
#include "voxel/Constants.h"
#include <memory>
#include <atomic>

#include "core/collection/ConcurrentQueue.h"
#include "core/ThreadPool.h"
#include "core/Var.h"
#include "core/GLM.h"
#include "math/Random.h"
#include <unordered_set>

namespace voxelworld {

struct ChunkMeshes {
	static constexpr bool MAY_GET_RESIZED = true;
	ChunkMeshes(int opaqueVertices, int opaqueIndices, int waterVertices, int waterIndices) :
			opaqueMesh(opaqueVertices, opaqueIndices, MAY_GET_RESIZED), waterMesh(waterVertices, waterIndices, MAY_GET_RESIZED) {
	}

	inline const glm::ivec3& translation() const {
		return opaqueMesh.getOffset();
	}

	voxel::Mesh opaqueMesh;
	voxel::Mesh waterMesh;

	inline bool operator<(const ChunkMeshes& rhs) const {
		return glm::all(glm::lessThan(translation(), rhs.translation()));
	}
};

typedef std::unordered_set<glm::ivec3, std::hash<glm::ivec3> > PositionSet;

/**
 * @brief The WorldMgr class is responsible to maintaining the voxel volumes and handle the needed mesh extraction
 * @ingroup Voxel
 */
class WorldMgr {
public:
	WorldMgr(const voxel::PagedVolume::PagerPtr& pager);
	~WorldMgr();

	/**
	 * @return true if the ray hit something - false if not.
	 * @note The callback has a parameter of @c const PagedVolume::Sampler& and returns a boolean. If the callback returns false,
	 * the ray is interrupted. Only if the callback returned false at some point in time, this function will return @c true.
	 */
	template<typename Callback>
	inline bool raycast(const glm::vec3& start, const glm::vec3& direction, float maxDistance, Callback&& callback) const {
		const voxel::RaycastResults::RaycastResult result = voxel::raycastWithDirection(_volumeData, start, direction * maxDistance, std::forward<Callback>(callback));
		return result == voxel::RaycastResults::Interupted;
	}

	template<typename VoxelTypeChecker>
	int findFloor(int x, int z, VoxelTypeChecker&& check) const {
		const glm::vec3 start(x, voxel::MAX_HEIGHT, z);
		const float distance = (float)voxel::MAX_HEIGHT;
		int y = voxel::NO_FLOOR_FOUND;
		raycast(start, glm::down, distance, [&] (const voxel::PagedVolume::Sampler& sampler) {
			if (check(sampler.voxel().getMaterial())) {
				y = sampler.position().y;
				return false;
			}
			return true;
		});
		return y;
	}

	/**
	 * @return The y component for the given x and z coordinates that is walkable - or @c NO_FLOOR_FOUND.
	 */
	int findWalkableFloor(const glm::vec3& position, float maxDistanceY = (float)voxel::MAX_HEIGHT) const;

	bool init(uint32_t volumeMemoryMegaBytes = 512, uint16_t chunkSideLength = 256);
	void shutdown();
	void reset();

	voxel::VoxelType material(int x, int y, int z) const;

	/**
	 * @brief Returns a random position inside the boundaries of the world (on the surface)
	 */
	glm::ivec3 randomPos() const;

	/**
	 * @brief We need to pop the mesh extractor queue to find out if there are new and ready to use meshes for us
	 * @return @c false if this isn't the case, @c true if the given reference was filled with valid data.
	 */
	bool pop(ChunkMeshes& item);

	void stats(int& meshes, int& extracted, int& pending) const;

	/**
	 * @brief If you don't need an extracted mesh anymore, make sure to allow the reextraction at a later time.
	 * @param[in] pos A world position vector that is automatically converted into a mesh tile vector
	 * @return @c true if the given position was already extracted, @c false if not.
	 */
	bool allowReExtraction(const glm::ivec3& pos);

	/**
	 * @brief Reorder the scheduled extraction commands that the closest chunks to the given position are handled first
	 */
	void updateExtractionOrder(const glm::ivec3& sortPos);

	/**
	 * @brief Performs async mesh extraction. You need to call @c pop in order to see if some extraction is ready.
	 *
	 * @param[in] pos A WorldMgr vector that is automatically converted into a mesh tile vector
	 * @note This will not allow to reschedule an extraction for the same area until @c allowReExtraction was called.
	 */
	bool scheduleMeshExtraction(const glm::ivec3& pos);

	long seed() const;

	void setSeed(long seed);

	bool created() const;

	glm::ivec3 meshSize() const;

	voxel::PagedVolume *volumeData();

	int chunkSize() const;

private:
	friend class WorldMgrTest;
	void extractScheduledMesh();

	/**
	 * @brief Cuts the given world coordinate down to mesh tile vectors
	 */
	glm::ivec3 meshPos(const glm::ivec3& pos) const;

	/**
	 * @brief Cuts the given world coordinate down to chunk tile vectors
	 */
	glm::ivec3 chunkPos(const glm::ivec3& pos) const;

	voxel::PagedVolume::PagerPtr _pager;
	voxel::PagedVolume *_volumeData = nullptr;
	mutable std::mt19937 _engine;
	long _seed = 0l;

	core::ThreadPool _threadPool;
	core::ConcurrentQueue<ChunkMeshes> _extracted;
	glm::ivec3 _pendingExtractionSortPosition = glm::zero<glm::ivec3>();
	struct CloseToPoint {
		glm::ivec3 _refPoint;
		CloseToPoint(const glm::ivec3& refPoint) : _refPoint(refPoint) {
		}
		inline int distanceToSortPos(const glm::ivec3 &pos) const {
			return glm::abs(pos.x - _refPoint.x) + glm::abs(pos.y - _refPoint.y) + glm::abs(pos.z - _refPoint.z);
		}
		inline bool operator()(const glm::ivec3& lhs, const glm::ivec3& rhs) const {
			return distanceToSortPos(lhs) > distanceToSortPos(rhs);
		}
	};
	core::ConcurrentQueue<glm::ivec3, CloseToPoint> _pendingExtraction { CloseToPoint(_pendingExtractionSortPosition) };
	// fast lookup for positions that are already extracted
	PositionSet _positionsExtracted;
	core::VarPtr _meshSize;
	math::Random _random;
	std::atomic_bool _cancelThreads { false };
};

inline voxel::PagedVolume *WorldMgr::volumeData() {
	return _volumeData;
}

inline glm::ivec3 WorldMgr::meshPos(const glm::ivec3& pos) const {
	const glm::vec3& size = meshSize();
	const int x = glm::floor(pos.x / size.x);
	const int y = glm::floor(pos.y / size.y);
	const int z = glm::floor(pos.z / size.z);
	return glm::ivec3(x * size.x, y * size.y, z * size.z);
}

inline glm::ivec3 WorldMgr::chunkPos(const glm::ivec3& pos) const {
	const float size = chunkSize();
	const int x = glm::floor(pos.x / size);
	const int y = glm::floor(pos.y / size);
	const int z = glm::floor(pos.z / size);
	return glm::ivec3(x, y, z);
}

inline bool WorldMgr::pop(ChunkMeshes& item) {
	return _extracted.pop(item);
}

inline bool WorldMgr::created() const {
	return _seed != 0;
}

inline long WorldMgr::seed() const {
	return _seed;
}

typedef std::shared_ptr<WorldMgr> WorldMgrPtr;

}
