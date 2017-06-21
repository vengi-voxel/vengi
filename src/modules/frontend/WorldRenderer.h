/**
 * @file
 */

#pragma once

#include "voxel/World.h"
#include "voxel/generator/PlantGenerator.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "video/Camera.h"
#include "video/VertexBuffer.h"
#include "video/UniformBuffer.h"
#include "video/GBuffer.h"
#include "video/DepthBuffer.h"
#include "FrontendShaders.h"
#include "core/GLM.h"
#include "core/Octree.h"
#include "core/Var.h"
#include "core/Color.h"
#include "ClientEntity.h"
#include "frontend/Shadow.h"
#include "frontend/RandomColorTexture.h"
#include "video/ShapeBuilder.h"
#include "frontend/ShapeRenderer.h"

#include <unordered_map>
#include <list>

namespace frontend {

/**
 * @brief Class that performs the rendering and extraction of the needed chunks.
 */
class WorldRenderer {
protected:
	struct PlantBuffer {
		~PlantBuffer() {
			shutdown();
		}
		void shutdown() {
			vb.shutdown();
			offsetBuffer = -1;
			indexBuffer = -1;
			vertexBuffer = -1;
			instancedPositions.clear();
		}
		int32_t offsetBuffer = -1;
		int32_t indexBuffer = -1;
		int32_t vertexBuffer = -1;
		uint32_t amount = 1u;
		video::VertexBuffer vb;
		std::vector<glm::vec3> instancedPositions;
	};

	struct ChunkBuffer {
		~ChunkBuffer() {
			core_assert(occlusionQueryId == video::InvalidId);
		}
		bool inuse = false;
		core::AABB<int> _aabb = {glm::zero<glm::ivec3>(), glm::zero<glm::ivec3>()};
		voxel::ChunkMeshes meshes {0, 0, 0, 0};
		std::vector<glm::vec3> instancedPositions;
		video::Id occlusionQueryId = video::InvalidId;
		bool occludedLastFrame = false;
		bool pendingResult = false;

		inline const glm::ivec3& translation() const {
			return meshes.opaqueMesh.getOffset();
		}

		inline const core::AABB<int>& aabb() const {
			return _aabb;
		}
	};

	core::Octree<ChunkBuffer*> _octree;
	static constexpr int MAX_CHUNKBUFFERS = 4096;
	ChunkBuffer _chunkBuffers[MAX_CHUNKBUFFERS];
	int _activeChunkBuffers = 0;
	int _visibleChunks = 0;
	int _occludedChunks = 0;
	int _queryResults = 0;
	PlantBuffer _meshPlantList[(int)voxel::PlantType::MaxPlantTypes];

	std::list<PlantBuffer*> _visiblePlant;
	std::vector<voxel::VoxelVertex> _opaqueVertices;
	std::vector<voxel::IndexType> _opaqueIndices;
	video::VertexBuffer _opaqueBuffer;
	int32_t _opaqueIbo = -1;
	int32_t _opaqueVbo = -1;
	std::vector<voxel::VoxelVertex> _waterVertices;
	std::vector<voxel::IndexType> _waterIndices;
	video::VertexBuffer _waterBuffer;
	int32_t _waterIbo = -1;
	int32_t _waterVbo = -1;

	typedef std::unordered_map<ClientEntityId, ClientEntityPtr> Entities;
	Entities _entities;

	Shadow _shadow;
	RandomColorTexture _colorTexture;
	voxel::PlantGenerator _plantGenerator;

	video::ShapeBuilder _shapeBuilder;
	frontend::ShapeRenderer _shapeRenderer;
	int32_t _aabbMeshes = -1;
	core::VarPtr _renderAABBs;
	core::VarPtr _occlusionThreshold;
	core::VarPtr _occlusionQuery;
	core::VarPtr _renderOccluded;

	video::ShapeBuilder _shapeBuilderOcclusionQuery;
	frontend::ShapeRenderer _shapeRendererOcclusionQuery;
	int32_t _aabbMeshesOcclusionQuery = -1;

	float _fogRange = 250.0f;
	// TODO: get the view distance from the server - entity attributes
	float _viewDistance;
	long _now = 0l;
	long _deltaFrame = 0l;

	glm::vec4 _clearColor = core::Color::LightBlue;
	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);
	voxel::WorldPtr _world;
	core::VarPtr _shadowMap;
	core::VarPtr _shadowMapShow;

	video::VertexBuffer _shadowMapDebugBuffer;
	video::DepthBuffer _depthBuffer;

	shader::Materialblock _materialBlock;
	shader::ShadowmapRenderShader _shadowMapRenderShader;
	shader::WorldShader _worldShader;
	shader::WorldInstancedShader _worldInstancedShader;
	shader::ShadowmapInstancedShader _shadowMapInstancedShader;
	shader::WaterShader _waterShader;
	shader::MeshShader _meshShader;
	shader::ShadowmapShader _shadowMapShader;

	/**
	 * @brief Convert a PolyVox mesh to OpenGL index/vertex buffers.
	 */
	bool createVertexBufferInternal(const video::Shader& shader, const voxel::Mesh &mesh, PlantBuffer& vbo);
	bool createInstancedVertexBuffer(const voxel::Mesh &mesh, int amount, PlantBuffer& vbo);
	void handleMeshQueue();
	void updateAABB(ChunkBuffer& chunkBuffer) const;
	/**
	 * @brief Redistribute the plants on the meshes that are already extracted
	 */
	void fillPlantPositionsFromMeshes();

	int getDistanceSquare(const glm::ivec3& pos, const glm::ivec3& pos2) const;

	void cull(const video::Camera& camera);
	bool occluded(ChunkBuffer * chunkBuffer) const;
	/**
	 * @return The amount of drawcalls
	 */
	int renderPlants(const std::list<PlantBuffer*>& vbos, int* vertices);
	bool renderOpaqueBuffers();
	bool renderWaterBuffers();
	ChunkBuffer* findFreeChunkBuffer();
	bool checkShaders() const;

	bool initOpaqueBuffer();
	bool initWaterBuffer();

public:
	WorldRenderer(const voxel::WorldPtr& world);
	~WorldRenderer();

	void reset();

	void onConstruct();
	bool init(const glm::ivec2& position, const glm::ivec2& dimension);
	void onRunning(const video::Camera& camera, long dt);
	void shutdown();

	/** @brief extract meshes around the given position */
	void extractMeshes(const glm::vec3& pos, int radius = 5);
	void extractMeshes(const video::Camera& camera);

	ClientEntityPtr getEntity(ClientEntityId id) const;
	bool addEntity(const ClientEntityPtr& entity);
	bool removeEntity(ClientEntityId id);

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

inline float WorldRenderer::getViewDistance() const {
	return _viewDistance;
}

inline void WorldRenderer::setViewDistance(float viewDistance) {
	_viewDistance = viewDistance;
}

}
