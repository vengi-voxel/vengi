/**
 * @file
 */

#pragma once

#include "AnimationShaders.h"
#include "VoxelrenderShaders.h"
#include "WorldMeshExtractor.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/ThreadPool.h"
#include "core/Var.h"
#include "core/collection/ConcurrentQueue.h"
#include "core/collection/List.h"
#include "frontend/ClientEntity.h"
#include "math/Octree.h"
#include "render/RandomColorTexture.h"
#include "render/Shadow.h"
#include "render/ShapeRenderer.h"
#include "render/Skybox.h"
#include "video/Buffer.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "video/Shader.h"
#include "video/ShapeBuilder.h"
#include "video/Texture.h"
#include "video/UniformBuffer.h"
#include "voxel/PagedVolume.h"

#include <unordered_map>

namespace voxelrender {

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
		ChunkMeshes meshes{0, 0, 0, 0};
		std::vector<glm::vec3> instancedPositions;
		video::Id occlusionQueryId = video::InvalidId;
		bool occludedLastFrame = false;
		bool pendingResult = false;

		/**
		 * This is the world position. Not the render positions. There is no scale
		 * applied here.
		 */
		inline const glm::ivec3 &translation() const {
			return meshes.opaqueMesh.getOffset();
		}

		/**
		 * This is the render aabb. There might be a scale applied here. So the mins of
		 * the AABB might not be at the position given by @c translation()
		 */
		inline const math::AABB<int> &aabb() const {
			return _aabb;
		}
	};

	using Tree = math::Octree<ChunkBuffer *>;
	Tree _octree;
	static constexpr int MAX_CHUNKBUFFERS = 4096;
	ChunkBuffer _chunkBuffers[MAX_CHUNKBUFFERS];
	int _activeChunkBuffers = 0;
	int _visibleChunks = 0;
	int _occludedChunks = 0;
	int _queryResults = 0;

	WorldMeshExtractor _meshExtractor;

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
	core::List<frontend::ClientEntity*> _visibleEntities;

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

	// this ub is currently shared between the world, world instanced and water shader
	shader::WorldData _materialBlock;
	shader::WorldShader _worldShader;
	shader::WorldInstancedShader _worldInstancedShader;
	shader::WaterShader _waterShader;
	shader::SkeletonShader _chrShader;
	shader::ShadowmapShader& _shadowMapShader;
	shader::SkeletonshadowmapShader& _skeletonShadowMapShader;
	shader::ShadowmapInstancedShader& _shadowMapInstancedShader;

	void handleMeshQueue();
	void updateAABB(ChunkBuffer &chunkBuffer) const;

	int getDistanceSquare(const glm::ivec3 &pos, const glm::ivec3 &pos2) const;

	void cull(const video::Camera &camera);
	bool occluded(ChunkBuffer *chunkBuffer) const;
	bool renderOpaqueBuffers();
	bool renderWaterBuffers();
	ChunkBuffer *findFreeChunkBuffer();

	bool initOpaqueBuffer();
	bool initWaterBuffer();

	void initFrameBuffer(const glm::ivec2 &dimensions);
	void shutdownFrameBuffer();

	int renderToFrameBuffer(const video::Camera &camera);

public:
	WorldRenderer();
	~WorldRenderer();

	void reset();

	void construct();
	bool init(voxel::PagedVolume *volume, const glm::ivec2 &position, const glm::ivec2 &dimension);
	void update(const video::Camera &camera, uint64_t dt);
	void shutdown();

	render::Shadow &shadow();
	video::FrameBuffer &frameBuffer();
	render::RandomColorTexture &colorTexture();

	void setSeconds(float seconds);

	void extractMesh(const glm::ivec3 &pos);
	void extractMeshes(const video::Camera &camera);

	frontend::ClientEntityPtr getEntity(frontend::ClientEntityId id) const;
	bool addEntity(const frontend::ClientEntityPtr &entity);
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

	void stats(Stats &stats) const;

	float getViewDistance() const;
	void setViewDistance(float viewDistance);

	int renderWorld(const video::Camera &camera, int *vertices = nullptr);
	int renderEntities(const video::Camera &camera);
};

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

inline render::Shadow &WorldRenderer::shadow() {
	return _shadow;
}

inline video::FrameBuffer &WorldRenderer::frameBuffer() {
	return _frameBuffer;
}

inline render::RandomColorTexture &WorldRenderer::colorTexture() {
	return _colorTexture;
}

}
