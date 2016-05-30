/**
 * @file
 */

#pragma once

#include "voxel/World.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "video/Camera.h"
#include "video/GBuffer.h"
#include "video/GLMeshData.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "core/Color.h"
#include "ClientEntity.h"

#include <unordered_map>
#include <list>

namespace frontend {

class WorldRenderer {
private:
	struct NoiseGenerationTask {
		NoiseGenerationTask(uint8_t *_buffer, int _width, int _height, int _depth) :
				buffer(_buffer), width(_width), height(_height), depth(_depth) {
		}
		// pointer to preallocated buffer that was hand into the noise task
		uint8_t *buffer;
		const int width;
		const int height;
		const int depth;
	};

	typedef std::future<NoiseGenerationTask> NoiseFuture;
	std::vector<NoiseFuture> _noiseFuture;

	// Index/vertex buffer data
	typedef std::list<video::GLMeshData> GLMeshDatas;
	GLMeshDatas _meshDataOpaque;
	GLMeshDatas _meshDataWater;

	typedef std::unordered_map<ClientEntityId, ClientEntityPtr> Entities;
	Entities _entities;

	float _fogRange = 250.0f;
	// TODO: get the view distance from the server - entity attributes
	float _viewDistance = 0.1f;
	long _now = 0l;

	video::TexturePtr _colorTexture;
	glm::vec3 _lightPos = glm::vec3(10000.0, 10000.0, 10000.0);
	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	// the position of the last extraction - we only care for x and z here
	glm::ivec3 _lastGridPosition;
	voxel::WorldPtr _world;
	video::GBuffer _gbuffer;
	core::VarPtr _debugGeometry;

	// Convert a PolyVox mesh to OpenGL index/vertex buffers.
	video::GLMeshData createMesh(video::Shader& shader, voxel::ChunkMeshData& mesh, bool opaque);
	void updateMesh(voxel::Mesh& surfaceMesh, video::GLMeshData& meshData);
	void handleMeshQueue(video::Shader& shader);

	// we might want to get an answer for this question in two contexts, once for 'should-i-render-this' and once for
	// 'should-i-create/destroy-the-mesh'.
	bool isDistanceCulled(int distance2, bool queryForRendering = true) const;
	int getDistance2(const glm::ivec3& pos) const;
	// schedule mesh extraction around the camera position on the grid with the given radius
	void extractMeshAroundCamera(int radius);

	int renderWorldMeshes(video::Shader& shader, const video::Camera& camera, bool opaque, int* vertices);

public:
	WorldRenderer(const voxel::WorldPtr& world);
	~WorldRenderer();

	void reset();

	void onInit(int width, int height);
	void onRunning(long now);
	void shutdown();

	// called to initialed the player position
	void onSpawn(const glm::vec3& pos, int initialExtractionRadius = 5);

	ClientEntityPtr getEntity(ClientEntityId id);
	bool addEntity(const ClientEntityPtr& entity);
	bool removeEntity(ClientEntityId id);

	void stats(int& meshes, int& extracted, int& pending) const;

	// world coordinates x/z
	void deleteMesh(const glm::ivec3& pos);

	float getViewDistance() const;

	void setVoxel(const glm::ivec3& pos, const voxel::Voxel& voxel);

	bool extractNewMeshes(const glm::vec3& position, bool force = false);
	int renderWorld(video::Shader& opaqueShader, video::Shader& waterShader, const video::Camera& camera, int* vertices = nullptr);
	int renderEntities(const video::ShaderPtr& shader, const video::Camera& camera);
};

inline float WorldRenderer::getViewDistance() const {
	return _viewDistance;
}

}
