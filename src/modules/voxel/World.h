#pragma once

#include <PolyVox/Mesh.h>
#include <glm/glm.hpp>
#include <memory>
#include <unordered_set>
#include <mutex>
#include <queue>
#include <random>
#include <chrono>

#include "io/Filesystem.h"
#include "WorldData.h"
#include "Voxel.h"
#include "BiomManager.h"
#include "Raycast.h"
#include "core/ThreadPool.h"
#include "core/ReadWriteLock.h"
#include "core/Var.h"
#include "core/Random.h"
#include "core/Log.h"

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

	// if clientData is true, additional data that is only useful for rendering is generated
	void setClientData(bool clientData) {
		_clientData = clientData;
	}

	void destroy();

	Result raycast(const glm::vec3& start, const glm::vec3& end, voxel::Raycast& raycast);
	bool findPath(const PolyVox::Vector3DInt32& start, const PolyVox::Vector3DInt32& end, std::list<PolyVox::Vector3DInt32>& listResult);
	int findFloor(int x, int z) const;
	int getMaterial(int x, int y, int z) const;

	/**
	 * @brief Returns a random position inside the boundaries of the world (on the surface)
	 */
	glm::ivec3 randomPos() const;

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

	inline int getChunkSize() const {
		return _chunkSize->intVal();
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

	const core::Random& random() const { return _random; }

	inline long seed() const { return _seed; }

	void setSeed(long seed) {
		Log::info("Seed is: %li", seed);
		_seed = seed;
		_random.setSeed(seed);
		_noiseSeedOffsetX = _random.randomf(-10000.0f, 10000.0f);
		_noiseSeedOffsetZ = _random.randomf(-10000.0f, 10000.0f);
	}

	inline bool isCreated() const {
		return _seed != 0;
	}

private:
	enum class TreeType {
		DOME,
		CONE,
		ELLIPSIS,
		CUBE,
		MAX
	};

	class Pager: public WorldData::Pager {
	private:
		World& _world;
	public:
		Pager(World& world) :
				_world(world) {
		}

		void pageIn(const PolyVox::Region& region, WorldData::Chunk* chunk) override;

		void pageOut(const PolyVox::Region& region, WorldData::Chunk* chunk) override;
	};

	Pager _pager;
	WorldData *_volumeData;
	BiomManager _biomManager;
	mutable std::mt19937 _engine;
	long _seed;
	bool _clientData;

	struct IVec2HashEquals {
		size_t operator()(const glm::ivec2& k) const {
			// TODO: find a better hash function - we have a lot of collisions here
			return std::hash<int>()(k.x) ^ std::hash<int>()(k.y);
		}

		bool operator()(const glm::ivec2& a, const glm::ivec2& b) const {
			return a.x == b.x && a.y == b.y;
		}
	};

	template<typename Func>
	inline auto locked(Func&& func) -> typename std::result_of<Func()>::type {
		if (_mutex.try_lock_for(std::chrono::milliseconds(5000))) {
			lockGuard lock(_mutex, std::adopt_lock_t());
			return func();
		}
		Log::warn("Most likely a deadlock in the world - execute without locking");
		return func();
	}

	template<typename Func>
	inline auto locked(Func&& func) const -> typename std::result_of<Func()>::type {
		if (_mutex.try_lock_for(std::chrono::milliseconds(5000))) {
			lockGuard lock(_mutex, std::adopt_lock_t());
			return func();
		}
		Log::warn("Most likely a deadlock in the world - execute without locking");
		return func();
	}

	static int findChunkFloor(int chunkSize, WorldData::Chunk* chunk, int x, int y);

	bool load(const PolyVox::Region& region, WorldData::Chunk* chunk);
	bool save(const PolyVox::Region& region, WorldData::Chunk* chunk);
	// don't access the volume in anything that is called here
	void create(const PolyVox::Region& region, WorldData::Chunk* chunk);

	// width and height are already squared - to prevent using sqrt
	static void createCirclePlane(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel);
	static void createEllipse(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createCone(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createDome(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createCube(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	static void createPlane(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, int width, int depth, const Voxel& voxel);

	static void addTree(const PolyVox::Region& region, WorldData::Chunk* chunk, const glm::ivec3& pos, TreeType type, int trunkHeight, int trunkWidth, int width, int depth, int height);
	static void createTrees(const PolyVox::Region& region, WorldData::Chunk* chunk, core::Random& random);
	glm::ivec2 randomPosWithoutHeight(const PolyVox::Region& region, core::Random& random, int border = 0);
	void createClouds(const PolyVox::Region& region, WorldData::Chunk* chunk, core::Random& random);
	static void createUnderground(const PolyVox::Region& region, WorldData::Chunk* chunk);

	core::ThreadPool _threadPool;
	using mutex = std::recursive_timed_mutex;
	mutable mutex _mutex;
	using lockGuard = std::lock_guard<mutex>;
	core::ReadWriteLock _rwLock;
	std::deque<DecodedMeshData> _meshQueue;
	// fast lookup for positions that are already extracted and available in the _meshData vector
	std::unordered_set<glm::ivec2, IVec2HashEquals> _meshesExtracted;
	core::VarPtr _chunkSize;
	core::Random _random;
	float _noiseSeedOffsetX;
	float _noiseSeedOffsetZ;
};

inline int World::getMaterial(int x, int y, int z) const {
	return _volumeData->getVoxel(x, y, z).getMaterial();
}

typedef std::shared_ptr<World> WorldPtr;

}
