#pragma once

#include <PolyVox/Mesh.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_set>
#include <mutex>
#include <queue>
#include <random>

#include "noise/AccidentalNoise.h"
#include "noise/WorldShapeNoise.h"
#include "noise/NoisePPNoise.h"
#include "noise/PerlinNoise.h"
#include "io/Filesystem.h"
#include "WorldData.h"
#include "WorldEvents.h"
#include "Voxel.h"
#include "Raycast.h"
#include "util/IProgressMonitor.h"
#include "core/ThreadPool.h"
#include "core/ReadWriteLock.h"
#include "core/Var.h"

namespace voxel {

class World {
public:
	enum Result {
		COMPLETED, ///< If the ray passed through the volume without being interupted
		INTERUPTED, ///< If the ray was interupted while travelling
		FAILED
	};

	World();
	~World();

	void create(long seed, int size, util::IProgressMonitor* progressMonitor = nullptr);
	bool load(long seed, util::IProgressMonitor* progressMonitor = nullptr);
	bool save(long seed);
	void destroy();

	inline bool isCreated() const {
		return _volumeData != nullptr;
	}

	Result raycast(const glm::vec3& start, const glm::vec3& end, voxel::Raycast& raycast);
	bool findPath(const PolyVox::Vector3DInt32& start, const PolyVox::Vector3DInt32& end, std::list<PolyVox::Vector3DInt32>& listResult);
	int findFloor(int x, int z) const;
	int getMaterial(int x, int y, int z) const;

	/**
	 * @brief Returns a random position inside the boundaries of the world (on the surface)
	 */
	glm::ivec3 randomPos(int border = 0) const;
	glm::ivec2 randomPosWithoutHeight(int border = 0) const;

	/**
	 * @brief Cuts the given world coordinate down to mesh tile vectors
	 */
	inline glm::ivec2 getGridPos(const glm::vec3& pos) {
		return getGridPos(glm::ivec2(static_cast<int>(pos.x), static_cast<int>(pos.z)));
	}

	/**
	 * @brief Cuts the given world coordinate down to mesh tile vectors
	 */
	inline glm::ivec2 getGridPos(const glm::ivec2& pos) const {
		const int size = _chunkSize->intVal();
		return glm::ivec2(pos.x / size * size, pos.y / size * size);
	}

	/**
	 * @brief We need to pop the mesh extractor queue to find out if there are new and ready to use meshes for us
	 */
	inline bool pop(DecodedMeshData& item) {
		core::ScopedWriteLock lock(_rwLock);
		if (_meshQueue.empty())
			return false;
		item = std::move(_meshQueue.front());
		_meshQueue.pop_front();
		return true;
	}

	/**
	 * @brief If you don't need an extracted mesh anymore, make sure to allow the reextraction at a later time.
	 * @param[in] pos A World vector that is automatically converted into a mesh tile vector
	 */
	void allowReExtraction(const glm::ivec2& pos);

	/**
	 * @brief Performs async mesh extraction. You need to call @c pop in order to see if some extraction is ready.
	 *
	 * @param[in] pos A World vector that is automatically converted into a mesh tile vector
	 * @note This will not allow to reschedule an extraction for the same area until @c allowReExtraction was called.
	 */
	void scheduleMeshExtraction(const glm::ivec2& pos);

	void onFrame(long dt);

	inline int size() const { return _size; }
	inline long seed() const { return _seed; }

private:
	enum class TreeType {
		DOME,
		CONE,
		ELLIPSIS
	};

	WorldData* _volumeData;
	mutable std::mt19937 _engine;
	long _seed;
	int _size;
	noise::NoisePPNoise _noise;
	noise::WorldShapeNoise _worldShapeNoise;

	struct IVec2HashEquals {
		size_t operator()(const glm::ivec2& k) const {
			// TODO: find a better hash function - we have a lot of collisions here
			return std::hash<int>()(k.x) ^ std::hash<int>()(k.y);
		}

		bool operator()(const glm::ivec2& a, const glm::ivec2& b) const {
			return a.x == b.x && a.y == b.y;
		}
	};

	// assumes that the mutex is already locked
	glm::ivec3 internalRandomPos(int border = 0) const;
	int internalFindFloor(int x, int y) const;

	void createCirclePlane(const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel);
	void createEllipse(const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	void createCone(const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	void createDome(const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	void createCube(const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	void createPlane(const glm::ivec3& pos, int width, int depth, const Voxel& voxel);

	void addTree(const glm::ivec3& pos, TreeType type, int trunkHeight = 10);
	void createTrees();
	void createClouds();
	void createUnderground();

	core::ThreadPool _threadPool;
	mutable std::mutex _mutex;
	core::ReadWriteLock _rwLock;
	std::deque<DecodedMeshData> _meshQueue;
	// fast lookup for positions that are already extracted and available in the _meshData vector
	std::unordered_set<glm::ivec2, IVec2HashEquals> _meshesExtracted;
	core::VarPtr _chunkSize;
};

inline int World::getMaterial(int x, int y, int z) const {
	return _volumeData->getVoxel(x, y, z).getMaterial();
}

typedef std::shared_ptr<World> WorldPtr;

}
