#pragma once

#include <glm/glm.hpp>
#include "polyvox/Mesh.h"
#include "polyvox/PagedVolume.h"
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <chrono>
#include <vector>
#include <atomic>

#include "WorldPersister.h"
#include "io/Filesystem.h"
#include "BiomManager.h"
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

	struct WorldContext {
		int landscapeNoiseOctaves = 1;
		float landscapeNoisePersistence = 0.1f;
		float landscapeNoiseFrequency = 0.01f;
		float landscapeNoiseAmplitude = 0.5f;

		int caveNoiseOctaves = 1;
		float caveNoisePersistence = 0.1f;
		float caveNoiseFrequency = 0.05f;
		float caveNoiseAmplitude = 0.1f;
		float caveDensityThreshold = 1.1f;

		int mountainNoiseOctaves = 2;
		float mountainNoisePersistence = 0.3f;
		float mountainNoiseFrequency = 0.00075f;
		float mountainNoiseAmplitude = 0.5f;
	};

	void setContext(const WorldContext& ctx) {
		_ctx = ctx;
	}

	// if clientData is true, additional data that is only useful for rendering is generated
	void setClientData(bool clientData) {
		_clientData = clientData;
	}

	void destroy();
	void reset();
	bool isReset() const;

	bool findPath(const glm::ivec3& start, const glm::ivec3& end, std::list<glm::ivec3>& listResult);
	int findFloor(int x, int z) const;
	int getMaterial(int x, int y, int z) const;

	void placeTree(const TreeContext& ctx);

	/**
	 * @brief Returns a random position inside the boundaries of the world (on the surface)
	 */
	glm::ivec3 randomPos() const;

	/**
	 * @brief Cuts the given world coordinate down to mesh tile vectors
	 */
	inline glm::ivec3 getGridPos(const glm::ivec3& pos) const {
		const int size = _chunkSize->intVal();
		return getChunkPos(pos) * size;
	}

	/**
	 * @brief Cuts the given world coordinate down to mesh tile vectors
	 */
	inline glm::ivec3 getChunkPos(const glm::ivec3& pos) const {
		const float size = _chunkSize->floatVal();
		const int x = glm::floor(pos.x / size);
		const int y = glm::floor(pos.y / size);
		const int z = glm::floor(pos.z / size);
		return glm::ivec3(x, y, z);
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
	 * @return @c true if the given position was already extracted, @c false if not.
	 */
	bool allowReExtraction(const glm::ivec3& pos);

	/**
	 * @brief Performs async mesh extraction. You need to call @c pop in order to see if some extraction is ready.
	 *
	 * @param[in] pos A World vector that is automatically converted into a mesh tile vector
	 * @note This will not allow to reschedule an extraction for the same area until @c allowReExtraction was called.
	 */
	bool scheduleMeshExtraction(const glm::ivec3& pos);

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
	class Pager: public PagedVolume::Pager {
	private:
		WorldPersister _worldPersister;
		World& _world;
	public:
		Pager(World& world) :
				_world(world) {
		}

		void erase(const Region& region, PagedVolume::Chunk* chunk);

		void pageIn(const Region& region, PagedVolume::Chunk* chunk) override;

		void pageOut(const Region& region, PagedVolume::Chunk* chunk) override;
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

	static int findChunkFloor(int chunkSize, PagedVolume::Chunk* chunk, int x, int y);

	// don't access the volume in anything that is called here
	void create(TerrainContext& ctx);

	void calculateAO(const Region& region);

	void setVolumeVoxel(TerrainContext& ctx, const glm::ivec3& pos, const Voxel& voxel);

	// width and height are already squared - to prevent using sqrt
	void createCirclePlane(TerrainContext& ctx, const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel);
	void createEllipse(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	void createCone(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	void createDome(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	void createCube(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel);
	void createPlane(TerrainContext& ctx, const glm::ivec3& pos, int width, int depth, const Voxel& voxel);

	void addTree(TerrainContext& ctx, const glm::ivec3& pos, TreeType type, int trunkHeight, int trunkWidth, int width, int depth, int height);
	void createTrees(TerrainContext& ctx);
	glm::ivec2 randomPosWithoutHeight(const Region& region, int border = 0);
	void createClouds(TerrainContext& ctx);
	void createUnderground(TerrainContext& ctx);

	void cleanupFutures();
	Region getRegion(const glm::ivec3& pos) const;
	inline bool isValidChunkPosition(TerrainContext& ctx, const glm::ivec3& pos) const;

	Pager _pager;
	PagedVolume *_volumeData;
	BiomManager _biomManager;
	WorldContext _ctx;
	mutable std::mt19937 _engine;
	long _seed;
	bool _clientData;

	core::ThreadPool _threadPool;
	using mutex = std::recursive_timed_mutex;
	mutable mutex _mutex;
	using lockGuard = std::lock_guard<mutex>;
	core::ReadWriteLock _rwLock;
	std::deque<DecodedMeshData> _meshQueue;
	// fast lookup for positions that are already extracted and available in the _meshData vector
	PositionSet _meshesExtracted;
	core::VarPtr _chunkSize;
	core::Random _random;
	std::vector<std::future<void> > _futures;
	std::atomic_bool _cancelThreads { false };
	float _noiseSeedOffsetX;
	float _noiseSeedOffsetZ;
};

inline int World::getMaterial(int x, int y, int z) const {
	return _volumeData->getVoxel(x, y, z).getMaterial();
}

typedef std::shared_ptr<World> WorldPtr;

}
