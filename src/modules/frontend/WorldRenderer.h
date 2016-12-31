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
#include "video/GLMeshData.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "core/Color.h"
#include "ClientEntity.h"
#include "frontend/Shadow.h"

#include <unordered_map>
#include <list>

namespace frontend {

/**
 * @brief Class that performs the rendering and extraction of the needed chunks.
 */
class WorldRenderer {
protected:
	struct NoiseGenerationTask {
		NoiseGenerationTask(uint8_t *_buffer, int _width, int _height, int _depth) :
				buffer(_buffer), width(_width), height(_height), depth(_depth) {
		}
		/** @brief pointer to preallocated buffer that was hand into the noise task */
		uint8_t *buffer;
		const int width;
		const int height;
		const int depth;
	};

	typedef std::future<NoiseGenerationTask> NoiseFuture;
	std::vector<NoiseFuture> _noiseFuture;

	struct GLChunkMeshData {
		video::GLMeshData opaque;
		video::GLMeshData water;
	};
	typedef std::list<video::GLMeshData> GLMeshDatas;
	typedef std::list<video::GLMeshData*> GLMeshesVisible;
	typedef std::list<GLChunkMeshData> GLMeshOpaqueDatas;

	GLMeshOpaqueDatas _meshData;
	GLMeshDatas _meshDataPlant;

	GLMeshesVisible _visible;
	GLMeshesVisible _visiblePlant;
	GLMeshesVisible _visibleWater;

	typedef std::unordered_map<ClientEntityId, ClientEntityPtr> Entities;
	Entities _entities;

	Shadow _shadow;
	float _fogRange = 250.0f;
	float _lineWidth = 2.0f;
	// TODO: get the view distance from the server - entity attributes
	float _viewDistance = 1.0f;
	long _now = 0l;
	long _deltaFrame = 0l;

	glm::vec4 _clearColor;
	video::TexturePtr _colorTexture;
	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	glm::vec3 _ambientColor = glm::vec3(0.2, 0.2, 0.2);
	/**
	 * @brief The position of the last extraction
	 * @note we only care for x and z here
	 */
	glm::ivec3 _lastGridPosition = { std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min(), std::numeric_limits<int32_t>::min() };
	voxel::WorldPtr _world;
	video::GBuffer _gbuffer;
	core::VarPtr _deferred;
	core::VarPtr _deferredDebug;
	core::VarPtr _shadowMap;
	core::VarPtr _shadowMapDebug;
	video::VertexBuffer _fullscreenQuad;
	video::VertexBuffer _shadowMapDebugBuffer;

	video::VertexBuffer _worldBuffer;
	int32_t _worldIndexBufferIndex = -1;
	int32_t _worldIndexBuffer = -1;

	video::VertexBuffer _worldInstancedBuffer;
	int32_t _worldInstancedIndexBufferIndex = -1;
	int32_t _worldInstancedBufferIndex = -1;
	int32_t _worldInstancedOffsetBufferIndex = -1;

	video::DepthBuffer _depthBuffer;

	shader::ShadowmapRenderShader _shadowMapDebugShader;
	shader::WorldShader _worldShader;
	shader::WorldInstancedShader _worldInstancedShader;
	shader::ShadowmapInstancedShader _shadowMapInstancedShader;
	shader::WaterShader _waterShader;
	shader::MeshShader _meshShader;
	shader::DeferredLightDirShader _deferredDirLightShader;
	shader::ShadowmapShader _shadowMapShader;

	voxel::PlantGenerator _plantGenerator;

	/**
	 * @brief Convert a PolyVox mesh to OpenGL index/vertex buffers.
	 */
	bool createMeshInternal(const video::Shader& shader, const voxel::Mesh &mesh, int buffers, video::GLMeshData& meshData);
	bool createMesh(const voxel::ChunkMeshData &mesh, GLChunkMeshData& meshData);
	bool createInstancedMesh(const voxel::Mesh &mesh, int amount, video::GLMeshData& meshData);
	void updateMesh(const voxel::Mesh& surfaceMesh, video::GLMeshData& meshData);
	void handleMeshQueue();
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
	void setUniforms(video::Shader& shader, const video::Camera& camera);
	int renderWorldMeshes(video::Shader& shader, const GLMeshesVisible& meshes, int* vertices);
	void renderWorldDeferred(const video::Camera& camera, const int width, const int height);

	bool checkShaders() const;

public:
	WorldRenderer(const voxel::WorldPtr& world);
	~WorldRenderer();

	void reset();

	void onConstruct();
	bool onInit(const glm::ivec2& position, const glm::ivec2& dimension);
	void onRunning(long dt);
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
