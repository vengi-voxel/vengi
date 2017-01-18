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
#include "core/Var.h"
#include "core/Color.h"
#include "ClientEntity.h"
#include "frontend/Shadow.h"
#include "frontend/RandomColorTexture.h"

#include <unordered_map>
#include <list>

namespace frontend {

/**
 * @brief Class that performs the rendering and extraction of the needed chunks.
 */
class WorldRenderer {
protected:
	struct ChunkBuffer {
		bool inuse = false;
		core::AABB<float> aabb = {glm::zero<glm::vec3>(), glm::zero<glm::vec3>()};
		voxel::ChunkMeshes meshes {0, 0, 0, 0};
		struct VBO {
			~VBO() {
				shutdown();
			}
			void shutdown() {
				vb.shutdown();
			}
			int32_t offsetBuffer = -1;
			int32_t indexBuffer = -1;
			int32_t vertexBuffer = -1;
			uint32_t amount = 1u;
			std::vector<glm::vec3> instancedPositions;
			video::VertexBuffer vb;
		};
		VBO opaque;
		VBO water;

		inline bool isActive() const {
			return opaque.vertexBuffer != -1;
		}

		inline const glm::ivec3& translation() const {
			return meshes.opaqueMesh.getOffset();
		}
	};
	typedef std::list<ChunkBuffer::VBO*> VisibleVBOs;

	static constexpr int MAX_CHUNKBUFFERS = 128;
	ChunkBuffer _chunkBuffers[MAX_CHUNKBUFFERS];
	int _activeChunkBuffers = 0;
	ChunkBuffer::VBO _meshPlantList[(int)voxel::PlantType::MaxPlantTypes];

	VisibleVBOs _visible;
	VisibleVBOs _visiblePlant;
	VisibleVBOs _visibleWater;

	typedef std::unordered_map<ClientEntityId, ClientEntityPtr> Entities;
	Entities _entities;

	Shadow _shadow;
	RandomColorTexture _colorTexture;
	voxel::PlantGenerator _plantGenerator;

	float _fogRange = 250.0f;
	// TODO: get the view distance from the server - entity attributes
	float _viewDistance = 1.0f;
	long _now = 0l;
	long _deltaFrame = 0l;

	glm::vec4 _clearColor;
	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);
	/**
	 * @brief The position of the last extraction
	 * @note we only care for x and z here
	 */
	glm::ivec3 _lastGridPosition = { std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min() };
	voxel::WorldPtr _world;
	core::VarPtr _shadowMap;
	core::VarPtr _shadowMapDebug;

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
	bool createVertexBufferInternal(const video::Shader& shader, const voxel::Mesh &mesh, ChunkBuffer::VBO& vbo);
	bool createVertexBuffer(const voxel::ChunkMeshes &mesh, ChunkBuffer& chunkBuffer);
	bool createInstancedVertexBuffer(const voxel::Mesh &mesh, int amount, ChunkBuffer::VBO& vbo);
	void updateVertexBuffer(const voxel::Mesh& surfaceMesh, ChunkBuffer::VBO& vbo) const;
	void handleMeshQueue();
	void updateAABB(ChunkBuffer& chunkBuffer) const;
	/**
	 * @brief Redistribute the plants on the meshes that are already extracted
	 */
	void fillPlantPositionsFromMeshes();

	int getDistanceSquare(const glm::ivec3& pos) const;
	/**
	 * @brief Schedule mesh extraction around the grid position with the given radius
	 */
	void extractMeshAroundCamera(const glm::ivec3& gridPos, int radius = 1);

	void cull(const video::Camera& camera);
	int renderWorldMeshes(video::Shader& shader, const VisibleVBOs& vbos, int* vertices);
	ChunkBuffer* findFreeChunkBuffer();
	bool checkShaders() const;

public:
	WorldRenderer(const voxel::WorldPtr& world);
	~WorldRenderer();

	void reset();

	void onConstruct();
	bool onInit(const glm::ivec2& position, const glm::ivec2& dimension);
	void onRunning(const video::Camera& camera, long dt);
	void shutdown();

	/** @brief called to initialed the player position */
	void onSpawn(const glm::vec3& pos, int initialExtractionRadius = 5);

	ClientEntityPtr getEntity(ClientEntityId id) const;
	bool addEntity(const ClientEntityPtr& entity);
	bool removeEntity(ClientEntityId id);

	void stats(int& meshes, int& extracted, int& pending, int& active) const;

	float getViewDistance() const;
	void setViewDistance(float viewDistance);

	bool extractNewMeshes(const glm::vec3& position, bool force = false);
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
