/**
 * @file
 */

#pragma once

#include "video/Shader.h"
#include "video/Texture.h"
#include "video/Camera.h"
#include "video/Buffer.h"
#include "video/UniformBuffer.h"
#include "video/FrameBuffer.h"
#include "VoxelrenderShaders.h"
#include "AnimationShaders.h"
#include "core/GLM.h"
#include "math/Octree.h"
#include "core/Var.h"
#include "core/ThreadPool.h"
#include "core/Color.h"
#include "core/collection/ConcurrentQueue.h"
#include "frontend/ClientEntity.h"
#include "render/Shadow.h"
#include "render/RandomColorTexture.h"
#include "video/ShapeBuilder.h"
#include "render/ShapeRenderer.h"
#include "render/Skybox.h"
#include "voxel/PagedVolume.h"

#include <unordered_map>
#include <unordered_set>

namespace voxelrender {

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
 * @brief Class that performs the rendering and extraction of the needed chunks.
 */
class WorldRenderer {
	friend class MapView;
protected:
	struct ChunkBuffer {
		~ChunkBuffer() {
			core_assert(occlusionQueryId == video::InvalidId);
		}
		bool inuse = false;
		math::AABB<int> _aabb = {glm::zero<glm::ivec3>(), glm::zero<glm::ivec3>()};
		ChunkMeshes meshes {0, 0, 0, 0};
		std::vector<glm::vec3> instancedPositions;
		video::Id occlusionQueryId = video::InvalidId;
		bool occludedLastFrame = false;
		bool pendingResult = false;

		/**
		 * This is the world position. Not the render positions. There is no scale
		 * applied here.
		 */
		inline const glm::ivec3& translation() const {
			return meshes.opaqueMesh.getOffset();
		}

		/**
		 * This is the render aabb. There might be a scale applied here. So the mins of
		 * the AABB might not be at the position given by @c translation()
		 */
		inline const math::AABB<int>& aabb() const {
			return _aabb;
		}
	};

	using Tree = math::Octree<ChunkBuffer*>;
	Tree _octree;
	static constexpr int MAX_CHUNKBUFFERS = 4096;
	ChunkBuffer _chunkBuffers[MAX_CHUNKBUFFERS];
	int _activeChunkBuffers = 0;
	int _visibleChunks = 0;
	int _occludedChunks = 0;
	int _queryResults = 0;

	std::vector<voxel::VoxelVertex> _opaqueVertices;
	std::vector<voxel::IndexType> _opaqueIndices;
	video::Buffer _opaqueBuffer;
	int32_t _opaqueIbo = -1;
	int32_t _opaqueVbo = -1;
	std::vector<voxel::VoxelVertex> _waterVertices;
	std::vector<voxel::IndexType> _waterIndices;
	video::Buffer _waterBuffer;
	int32_t _waterIbo = -1;
	int32_t _waterVbo = -1;
	int _maxAllowedDistance = -1;

	typedef std::unordered_map<frontend::ClientEntityId, frontend::ClientEntityPtr> Entities;
	Entities _entities;

	glm::vec3 _focusPos = glm::zero<glm::vec3>();
	render::Shadow _shadow;
	render::RandomColorTexture _colorTexture;

	render::Skybox _skybox;

	video::FrameBuffer _frameBuffer;
	shader::PostprocessShader _postProcessShader;
	video::Buffer _postProcessBuf;
	int32_t _postProcessBufId = -1;

	video::ShapeBuilder _shapeBuilder;
	render::ShapeRenderer _shapeRenderer;
	int32_t _aabbMeshes = -1;
	core::VarPtr _renderAABBs;
	core::VarPtr _occlusionThreshold;
	core::VarPtr _occlusionQuery;
	core::VarPtr _renderOccluded;

	video::ShapeBuilder _shapeBuilderOcclusionQuery;
	render::ShapeRenderer _shapeRendererOcclusionQuery;
	int32_t _aabbMeshesOcclusionQuery = -1;

	float _fogRange;
	float _viewDistance;
	uint64_t _now = 0ul;
	uint64_t _deltaFrame = 0ul;
	float _seconds = 0.0f;

	glm::vec4 _clearColor = core::Color::LightBlue;
	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);
	glm::vec3 _nightColor = glm::vec3(0.001, 0.001, 0.2);
	core::VarPtr _shadowMap;
	core::VarPtr _shadowMapShow;
	voxel::PagedVolume *_volume;

	// this ub is currently shared between the world, world instanced and water shader
	shader::WorldData _materialBlock;
	shader::WorldShader _worldShader;
	shader::WorldInstancedShader _worldInstancedShader;
	shader::WaterShader _waterShader;
	shader::SkeletonShader _chrShader;

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
	std::atomic_bool _cancelThreads { false };
	void extractScheduledMesh();

	/**
	 * @brief Cuts the given world coordinate down to mesh tile vectors
	 */
	glm::ivec3 meshPos(const glm::ivec3& pos) const;

	/**
	 * @brief We need to pop the mesh extractor queue to find out if there are new and ready to use meshes for us
	 * @return @c false if this isn't the case, @c true if the given reference was filled with valid data.
	 */
	bool pop(ChunkMeshes& item);

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

	glm::ivec3 meshSize() const;

	void handleMeshQueue();
	void updateAABB(ChunkBuffer& chunkBuffer) const;

	int getDistanceSquare(const glm::ivec3& pos, const glm::ivec3& pos2) const;

	void cull(const video::Camera& camera);
	bool occluded(ChunkBuffer * chunkBuffer) const;
	bool renderOpaqueBuffers();
	bool renderWaterBuffers();
	ChunkBuffer* findFreeChunkBuffer();

	bool initOpaqueBuffer();
	bool initWaterBuffer();

	void initFrameBuffer(const glm::ivec2& dimensions);
	void shutdownFrameBuffer();

	int renderToFrameBuffer(const video::Camera& camera);

public:
	WorldRenderer();
	~WorldRenderer();

	void reset();

	void construct();
	bool init(voxel::PagedVolume* volume, const glm::ivec2& position, const glm::ivec2& dimension);
	void update(const video::Camera& camera, uint64_t dt);
	void shutdown();

	render::Shadow& shadow();

	void setSeconds(float seconds);

	void extractMesh(const glm::ivec3& pos);
	void extractMeshes(const video::Camera& camera);

	frontend::ClientEntityPtr getEntity(frontend::ClientEntityId id) const;
	bool addEntity(const frontend::ClientEntityPtr& entity);
	bool removeEntity(frontend::ClientEntityId id);

	struct Stats {
		int meshes = 0;
		int extracted = 0;
		int pending = 0;
		int active = 0;
		int visible = 0;
		int occluded = 0;
		int octreeSize = 0;
		int octreeActive = 0;
	};

	void stats(Stats& stats) const;

	float getViewDistance() const;
	void setViewDistance(float viewDistance);

	int renderWorld(const video::Camera& camera, int* vertices = nullptr);
	int renderEntities(const video::Camera& camera);
};

inline bool WorldRenderer::pop(ChunkMeshes& item) {
	return _extracted.pop(item);
}

inline glm::ivec3 WorldRenderer::meshPos(const glm::ivec3& pos) const {
	const glm::vec3& size = meshSize();
	const int x = glm::floor(pos.x / size.x);
	const int y = glm::floor(pos.y / size.y);
	const int z = glm::floor(pos.z / size.z);
	return glm::ivec3(x * size.x, y * size.y, z * size.z);
}

inline void WorldRenderer::setSeconds(float seconds) {
	_seconds = seconds;
}

inline float WorldRenderer::getViewDistance() const {
	return _viewDistance;
}

inline void WorldRenderer::setViewDistance(float viewDistance) {
	_viewDistance = viewDistance;
	_fogRange = _viewDistance * 0.80f;
}

inline render::Shadow& WorldRenderer::shadow() {
	return _shadow;
}

}
