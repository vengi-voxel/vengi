/**
 * @file
 */

#pragma once

#include "voxel/Mesh.h"
#include "core/concurrent/ThreadPool.h"
#include "core/Var.h"
#include "core/collection/ConcurrentQueue.h"
#include "voxel/PagedVolume.h"
#include "core/concurrent/Atomic.h"

#include <unordered_set>
#include <glm/vec3.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace voxelrender {

typedef std::unordered_set<glm::ivec3, std::hash<glm::ivec3> > PositionSet;

class WorldMeshExtractor {
private:
	core::ConcurrentQueue<voxel::Mesh> _extracted;
	glm::ivec3 _pendingExtractionSortPosition { 0, 0, 0 };
	struct CloseToPoint {
		glm::ivec2 _refPoint;
		CloseToPoint(const glm::ivec3& refPoint) : _refPoint(refPoint.x, refPoint.z) {
		}
		inline int distanceToSortPos(const glm::ivec3 &pos) const {
			const glm::ivec2 d(_refPoint.x - pos.x, _refPoint.y - pos.z);
			return d.x * d.x + d.y * d.y;
		}
		inline bool operator()(const glm::ivec3& lhs, const glm::ivec3& rhs) const {
			return distanceToSortPos(lhs) > distanceToSortPos(rhs);
		}
	};

	core::ConcurrentQueue<glm::ivec3, CloseToPoint> _pendingExtraction { CloseToPoint(_pendingExtractionSortPosition) };
	// fast lookup for positions that are already extracted
	PositionSet _positionsExtracted;
	core::VarPtr _meshSize;
	voxel::PagedVolume *_volume = nullptr;

public:
	WorldMeshExtractor();

	void extractScheduledMesh();

	/**
	 * @brief We need to pop the mesh extractor queue to find out if there are new and ready to use meshes for us
	 * @return @c false if this isn't the case, @c true if the given reference was filled with valid data.
	 */
	bool pop(voxel::Mesh& item);

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
	 * @param[in] pos A world vector that is automatically converted into a mesh tile vector
	 * @note This will not allow to reschedule an extraction for the same area until @c allowReExtraction was called.
	 */
	bool scheduleMeshExtraction(const glm::ivec3& pos);

	void reset();

	/**
	 * @brief Cuts the given world coordinate down to mesh tile vectors
	 */
	glm::ivec3 meshPos(const glm::ivec3& pos) const;

	glm::ivec3 meshSize() const;

	bool init(voxel::PagedVolume *volume);
	void shutdown();
};

}