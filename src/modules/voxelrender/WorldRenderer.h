/**
 * @file
 */

#pragma once

#include "RenderShaders.h"
#include "AnimationShaders.h"
#include "VoxelrenderShaders.h"
#include "worldrenderer/WorldChunkMgr.h"
#include "worldrenderer/WorldBuffers.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "core/collection/List.h"
#include "frontend/ClientEntityRenderer.h"
#include "frontend/EntityMgr.h"
#include "render/RandomColorTexture.h"
#include "render/Shadow.h"
#include "render/Skybox.h"
#include "video/Camera.h"
#include "video/FrameBuffer.h"
#include "video/UniformBuffer.h"
#include "voxel/PagedVolume.h"

namespace voxelrender {

/**
 * @brief Class that performs the rendering and extraction of the needed chunks.
 */
class WorldRenderer {
protected:
	core::ThreadPool _threadPool;
	core::AtomicBool _cancelThreads { false };

	WorldChunkMgr _worldChunkMgr;
	WorldBuffers _worldBuffers;
	frontend::EntityMgr _entityMgr;

	render::Shadow _shadow;
	render::RandomColorTexture _colorTexture;
	video::TexturePtr _distortionTexture;
	video::TexturePtr _normalTexture;
	render::Skybox _skybox;
	frontend::ClientEntityRenderer _entityRenderer;

	video::FrameBuffer _frameBuffer;
	video::FrameBuffer _reflectionBuffer;
	video::FrameBuffer _refractionBuffer;
	shader::PostprocessShader _postProcessShader;
	video::Buffer _postProcessBuf;
	int32_t _postProcessBufId = -1;

	float _fogRange = 0.0f;
	float _viewDistance = 0.0f;
	float _seconds = 0.0f;
	glm::vec3 _focusPos { 0.0f };

	core::VarPtr _shadowMap;

	// this ub is currently shared between the world, world instanced and water shader
	shader::WorldData _materialBlock;
	// dedicated shaders
	shader::WorldShader _worldShader;
	shader::WaterShader _waterShader;
	// shared shaders
	shader::ShadowmapShader& _shadowMapShader;

	bool initFrameBuffers(const glm::ivec2 &dimensions);
	void shutdownFrameBuffers();

	glm::mat4 waterModelMatrix() const;
	/**
	 * @brief Calculate the reflection matrix for a upwards pointing water plane normal
	 * @sa Goldman 1990
	 */
	glm::mat4 reflectionMatrix(const video::Camera& camera) const;

	/**
	 * @brief Render the whole scene to a framebuffer to apply post processing effects afterwards
	 */
	int renderToFrameBuffer(const video::Camera &camera);
	int renderToShadowMap(const video::Camera& camera);

	int renderEntitiesToDepthMap(const video::Camera& camera);

	int renderAll(const video::Camera& camera);
	int renderTerrain(const glm::mat4& viewProjectionMatrix, const glm::vec4& clipPlane);
	int renderEntities(const glm::mat4& viewProjectionMatrix, const glm::vec4& clipPlane);
	int renderEntityDetails(const video::Camera& camera);
	int renderWater(const video::Camera& camera, const glm::vec4& clipPlane);

	/**
	 * @brief 2-pass render of the reflection and the refraction buffers
	 */
	int renderClippingPlanes(const video::Camera& camera);
	/**
	 * @sa shader::PostprocessShader
	 */
	int renderPostProcessEffects(const video::Camera& camera);
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
	video::FrameBuffer &reflectionBuffer();
	video::FrameBuffer &refractionBuffer();
	video::FrameBuffer &entitiesBuffer();
	render::RandomColorTexture &colorTexture();

	frontend::EntityMgr &entityMgr();
	const frontend::EntityMgr &entityMgr() const;

	void setSeconds(float seconds);

	void extractMesh(const glm::ivec3 &pos);
	void extractMeshes(const video::Camera &camera);

	float getViewDistance() const;
	void setViewDistance(float viewDistance);

	int renderWorld(const video::Camera &camera);
};

inline frontend::EntityMgr &WorldRenderer::entityMgr() {
	return _entityMgr;
}

inline const frontend::EntityMgr &WorldRenderer::entityMgr() const {
	return _entityMgr;
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
	_entityRenderer.setViewDistance(_viewDistance, _fogRange);
}

inline render::Shadow &WorldRenderer::shadow() {
	return _shadow;
}

inline video::FrameBuffer &WorldRenderer::frameBuffer() {
	return _frameBuffer;
}

inline video::FrameBuffer &WorldRenderer::reflectionBuffer() {
	return _reflectionBuffer;
}

inline video::FrameBuffer &WorldRenderer::refractionBuffer() {
	return _refractionBuffer;
}

inline video::FrameBuffer &WorldRenderer::entitiesBuffer() {
	return _entityRenderer.entitiesBuffer();
}

inline render::RandomColorTexture &WorldRenderer::colorTexture() {
	return _colorTexture;
}

}
