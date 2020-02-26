/**
 * @file
 */

#include "WorldRenderer.h"
#include "WorldShaderConstants.h"

#include "core/Color.h"
#include "core/GameConfig.h"
#include "core/ArrayLength.h"
#include "core/Assert.h"
#include "core/Var.h"
#include "core/Trace.h"
#include "core/GLM.h"
#include "voxel/Constants.h"
#include "voxel/MaterialColor.h"

#include "video/Renderer.h"
#include "video/ScopedState.h"
#include "video/Types.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace voxelrender {

// TODO: respect max vertex/index size of the one-big-vbo/ibo
WorldRenderer::WorldRenderer() :
		_shadowMapShader(shader::ShadowmapShader::getInstance()),
		_skeletonShadowMapShader(shader::SkeletonshadowmapShader::getInstance()) {
	setViewDistance(240.0f);
}

WorldRenderer::~WorldRenderer() {
}

void WorldRenderer::reset() {
	_worldChunkMgr.reset();
	_entityMgr.reset();
}

void WorldRenderer::shutdown() {
	_worldShader.shutdown();
	_waterShader.shutdown();
	_chrShader.shutdown();
	_materialBlock.shutdown();
	reset();
	_worldChunkMgr.shutdown();
	_colorTexture.shutdown();
	if (_normalTexture) {
		_normalTexture->shutdown();
	}
	_worldBuffers.shutdown();
	_shadow.shutdown();
	_skybox.shutdown();
	_shadowMapShader.shutdown();
	_skeletonShadowMapShader.shutdown();
	shutdownFrameBuffers();
	_postProcessBuf.shutdown();
	_postProcessBufId = -1;
	_postProcessShader.shutdown();
}

int WorldRenderer::renderWorld(const video::Camera& camera) {
	core_trace_scoped(WorldRendererRenderWorld);
	_worldChunkMgr.handleMeshQueue();
	_worldChunkMgr.cull(camera);
	int drawCallsWorld = 0;
	drawCallsWorld += renderToFrameBuffer(camera);
	drawCallsWorld += renderPostProcessEffects(camera);
	return drawCallsWorld;
}

int WorldRenderer::renderPostProcessEffects(const video::Camera& camera) {
	video::ScopedState depthTest(video::State::DepthTest, false);
	const video::TexturePtr& fboTexture = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
	video::ScopedShader scoped(_postProcessShader);
	video::ScopedTexture scopedTex(fboTexture, video::TextureUnit::Zero);
	video::ScopedBuffer scopedBuf(_postProcessBuf);
	const int currentEyeHeight = (int)camera.eye().y;
	if (currentEyeHeight <= voxel::MAX_WATER_HEIGHT) {
		static const voxel::Voxel waterVoxel = voxel::createColorVoxel(voxel::VoxelType::Water, 0);
		const glm::vec4& waterColor = voxel::getMaterialColor(waterVoxel);
		_postProcessShader.setColor(waterColor);
	} else {
		_postProcessShader.setColor(glm::one<glm::vec4>());
	}
	_postProcessShader.setTexture(video::TextureUnit::Zero);
	const int elements = _postProcessBuf.elements(_postProcessBufId, _postProcessShader.getComponentsPos());
	video::drawArrays(video::Primitive::Triangles, elements);
	return 1;
}

video::Camera WorldRenderer::reflectionCamera(const video::Camera& camera) const {
	video::Camera reflectionCamera = camera;
	const float waterHeight = (float)voxel::MAX_WATER_HEIGHT;
	glm::vec3 position = camera.position();
	const float distance = 2.0f * (position.y - waterHeight);
	// TODO: positions lower than 0 only see earth from below
	// as our mesh extractor adds a bottom plane to the mesh
	position.y = glm::max(waterHeight, position.y - distance);
	reflectionCamera.setPosition(position);
	reflectionCamera.setPitch(-camera.pitch());
	reflectionCamera.update(0ul);
	return reflectionCamera;
}

int WorldRenderer::renderClippingPlanes(const video::Camera& camera) {
	int drawCallsWorld = 0;
	video::ScopedState scopedClipDistance(video::State::ClipDistance, true);
	const float waterHeight = (float)voxel::MAX_WATER_HEIGHT;
	constexpr glm::vec4 waterClipPlane(glm::down, waterHeight);

	// render above water
	_reflectionBuffer.bind(true);
	const glm::vec3& position = camera.position();
	if (position.y > waterHeight) {
		const video::Camera& reflCamera = reflectionCamera(camera);
		drawCallsWorld += renderTerrain(reflCamera, -waterClipPlane);
		drawCallsWorld += renderEntities(reflCamera);
	}
	_reflectionBuffer.unbind();

	// render below water
	_refractionBuffer.bind(true);
	drawCallsWorld += renderTerrain(camera, waterClipPlane);
	drawCallsWorld += renderEntities(camera);
	_refractionBuffer.unbind();

	return drawCallsWorld;
}

int WorldRenderer::renderToShadowMap(const video::Camera& camera) {
	if (!_shadowMap->boolVal()) {
		return 0;
	}
	core_trace_scoped(WorldRendererRenderShadow);
	_skeletonShadowMapShader.activate();
	_shadow.render([this] (int i, const glm::mat4& lightViewProjection) {
		_skeletonShadowMapShader.setLightviewprojection(lightViewProjection);
		for (const auto& ent : _entityMgr.visibleEntities()) {
			_skeletonShadowMapShader.setBones(ent->bones()._items);
			_skeletonShadowMapShader.setModel(ent->modelMatrix());
			const uint32_t numIndices = ent->bindVertexBuffers(_chrShader);
			video::drawElements<animation::IndexType>(video::Primitive::Triangles, numIndices);
			ent->unbindVertexBuffers();
		}
		return true;
	}, true);
	_skeletonShadowMapShader.deactivate();

	_shadowMapShader.activate();
	_shadowMapShader.setModel(glm::mat4(1.0f));
	_shadow.render([this] (int i, const glm::mat4& lightViewProjection) {
		_shadowMapShader.setLightviewprojection(lightViewProjection);
		_worldBuffers.renderOpaqueBuffers();
		return true;
	}, false);
	_shadowMapShader.deactivate();
	return (int)_entityMgr.visibleEntities().size() + 1;
}

int WorldRenderer::renderToFrameBuffer(const video::Camera& camera) {
	core_assert_always(_worldBuffers._opaqueBuffer.update(_worldBuffers._opaqueVbo, _worldChunkMgr._opaqueVertices));
	core_assert_always(_worldBuffers._opaqueBuffer.update(_worldBuffers._opaqueIbo, _worldChunkMgr._opaqueIndices));

	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);
	video::colorMask(true, true, true, true);

	int drawCallsWorld = 0;
	drawCallsWorld += renderToShadowMap(camera);

	_shadow.bind(video::TextureUnit::One);
	_colorTexture.bind(video::TextureUnit::Zero);
	video::clearColor(_clearColor);

	drawCallsWorld += renderClippingPlanes(camera);

	_frameBuffer.bind(true);
	// due to driver bugs the clip plane might still be taken into account - set to very high y value
	constexpr glm::vec4 ignoreClipPlane(glm::up, 100000.0f);
	drawCallsWorld += renderAll(camera, ignoreClipPlane);

	_skybox.render(camera);
	video::bindVertexArray(video::InvalidId);
	_colorTexture.unbind();

	_frameBuffer.unbind();

	return drawCallsWorld;
}

int WorldRenderer::renderTerrain(const video::Camera& camera, const glm::vec4& clipPlane) {
	const bool shadowMap = _shadowMap->boolVal();
	int drawCallsWorld = 0;
	core_trace_scoped(WorldRendererRenderOpaque);
	video::ScopedShader scoped(_worldShader);
	_worldShader.setModel(glm::mat4(1.0f));
	_worldShader.setMaterialblock(_materialBlock);
	_worldShader.setFocuspos(_focusPos);
	_worldShader.setLightdir(_shadow.sunDirection());
	_worldShader.setFogcolor(_clearColor);
	_worldShader.setTexture(video::TextureUnit::Zero);
	_worldShader.setDiffuseColor(_diffuseColor);
	_worldShader.setAmbientColor(_ambientColor);
	_worldShader.setNightColor(_nightColor);
	_worldShader.setTime(_seconds);
	_worldShader.setFogrange(_fogRange);
	_worldShader.setClipplane(clipPlane);
	if (shadowMap) {
		_worldShader.setViewprojection(camera.viewProjectionMatrix());
		_worldShader.setShadowmap(video::TextureUnit::One);
		_worldShader.setDepthsize(glm::vec2(_shadow.dimension()));
		_worldShader.setCascades(_shadow.cascades());
		_worldShader.setDistances(_shadow.distances());
	}
	if (_worldBuffers.renderOpaqueBuffers()) {
		++drawCallsWorld;
	}
	return drawCallsWorld;
}

int WorldRenderer::renderWater(const video::Camera& camera, const glm::vec4& clipPlane) {
	int drawCallsWorld = 0;
	const bool shadowMap = _shadowMap->boolVal();
	core_trace_scoped(WorldRendererRenderWater);
	video::ScopedShader scoped(_waterShader);
	constexpr glm::vec3 translate(0.0f, ((float)voxel::MAX_WATER_HEIGHT) - 0.05f, 0.0f);
	const glm::mat4& model = glm::scale(glm::translate(glm::mat4(1.0f), translate), glm::vec3(1000.0f));
	_waterShader.setModel(model);
	_waterShader.setFocuspos(_focusPos);
	_waterShader.setCamerapos(camera.position());
	_waterShader.setLightdir(_shadow.sunDirection());
	_waterShader.setFogcolor(_clearColor);
	_waterShader.setDiffuseColor(_diffuseColor);
	_waterShader.setAmbientColor(_ambientColor);
	_waterShader.setNightColor(_nightColor);
	_waterShader.setFogrange(_fogRange);
	_waterShader.setTime(_seconds);
	_skybox.bind(video::TextureUnit::Two);
	_waterShader.setCubemap(video::TextureUnit::Two);
	_reflectionBuffer.texture()->bind(video::TextureUnit::Three);
	_waterShader.setReflection(video::TextureUnit::Three);
	_refractionBuffer.texture()->bind(video::TextureUnit::Four);
	_waterShader.setRefraction(video::TextureUnit::Four);
	_distortionTexture->bind(video::TextureUnit::Five);
	_waterShader.setDistortion(video::TextureUnit::Five);
	_normalTexture->bind(video::TextureUnit::Six);
	_waterShader.setNormalmap(video::TextureUnit::Six);
	if (shadowMap) {
		_waterShader.setViewprojection(camera.viewProjectionMatrix());
		_waterShader.setShadowmap(video::TextureUnit::One);
		_waterShader.setDepthsize(glm::vec2(_shadow.dimension()));
		_waterShader.setCascades(_shadow.cascades());
		_waterShader.setDistances(_shadow.distances());
	}
	if (_worldBuffers.renderWaterBuffers()) {
		++drawCallsWorld;
	}
	_skybox.unbind(video::TextureUnit::Two);
	_normalTexture->unbind();
	_distortionTexture->unbind();
	_refractionBuffer.texture()->unbind();
	_reflectionBuffer.texture()->unbind();
	return drawCallsWorld;
}

int WorldRenderer::renderAll(const video::Camera& camera, const glm::vec4& clipPlane) {
	int drawCallsWorld = 0;
	drawCallsWorld += renderTerrain(camera, clipPlane);
	drawCallsWorld += renderEntities(camera);
	drawCallsWorld += renderWater(camera, clipPlane);
	return drawCallsWorld;
}

int WorldRenderer::renderEntities(const video::Camera& camera) {
	if (_entityMgr.visibleEntities().empty()) {
		return 0;
	}
	core_trace_gl_scoped(WorldRendererRenderEntities);

	int drawCallsEntities = 0;

	video::enable(video::State::DepthTest);
	video::enable(video::State::DepthMask);
	video::ScopedShader scoped(_chrShader);
	_chrShader.setFogrange(_fogRange);
	_chrShader.setFocuspos(_focusPos);
	_chrShader.setDiffuseColor(_diffuseColor);
	_chrShader.setAmbientColor(_ambientColor);
	_chrShader.setFogcolor(_clearColor);
	_chrShader.setLightdir(_shadow.sunDirection());
	_chrShader.setNightColor(_nightColor);
	_chrShader.setTime(_seconds);
	_chrShader.setMaterialblock(_materialBlock);

	const bool shadowMap = _shadowMap->boolVal();
	if (shadowMap) {
		_chrShader.setDepthsize(glm::vec2(_shadow.dimension()));
		_chrShader.setViewprojection(camera.viewProjectionMatrix());
		_chrShader.setShadowmap(video::TextureUnit::One);
		_chrShader.setCascades(_shadow.cascades());
		_chrShader.setDistances(_shadow.distances());
	}
	for (frontend::ClientEntity* ent : _entityMgr.visibleEntities()) {
		_chrShader.setModel(ent->modelMatrix());
		core_assert_always(_chrShader.setBones(ent->bones()._items));
		const uint32_t numIndices = ent->bindVertexBuffers(_chrShader);
		++drawCallsEntities;
		video::drawElements<animation::IndexType>(video::Primitive::Triangles, numIndices);
		ent->unbindVertexBuffers();
	}
	return drawCallsEntities;
}

void WorldRenderer::construct() {
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
}

bool WorldRenderer::init(voxel::PagedVolume* volume, const glm::ivec2& position, const glm::ivec2& dimension) {
	core_trace_scoped(WorldRendererOnInit);
	_colorTexture.init();

	_distortionTexture = video::createTextureFromImage("water-distortion.png");
	if (!_distortionTexture || !_distortionTexture->isLoaded()) {
		Log::error("Failed to load distortion texture");
		return false;
	}

	_normalTexture = video::createTextureFromImage("water-normal.png");
	if (!_normalTexture || !_normalTexture->isLoaded()) {
		Log::error("Failed to load normalmap texture");
		return false;
	}

	if (!_worldShader.setup()) {
		Log::error("Failed to setup the post world shader");
		return false;
	}
	if (!_waterShader.setup()) {
		Log::error("Failed to setup the post water shader");
		return false;
	}
	if (!_chrShader.setup()) {
		Log::error("Failed to setup the post skeleton shader");
		return false;
	}
	if (!_postProcessShader.setup()) {
		Log::error("Failed to setup the post processing shader");
		return false;
	}
	if (!_skybox.init("sky")) {
		Log::error("Failed to initialize the sky");
		return false;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return false;
	}
	if (!_skeletonShadowMapShader.setup()) {
		Log::error("Failed to init skeleton shadowmap shader");
		return false;
	}
	const int shaderMaterialColorsArraySize = lengthof(shader::WorldData::MaterialblockData::materialcolor);
	const int materialColorsArraySize = (int)voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::WorldData::MaterialblockData materialBlock;
	memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	_materialBlock.create(materialBlock);

	if (!_worldBuffers.init(_worldShader, _waterShader)) {
		return false;
	}

	render::ShadowParameters sp;
	sp.maxDepthBuffers = _worldShader.getUniformArraySize(shader::WorldShaderConstants::getMaxDepthBufferUniformName());
	if (!_shadow.init(sp)) {
		return false;
	}

	_worldChunkMgr.init(volume);
	_worldChunkMgr.updateViewDistance(_viewDistance);

	initFrameBuffers(dimension);
	_postProcessBufId = _postProcessBuf.createFullscreenTextureBufferYFlipped();
	if (_postProcessBufId == -1) {
		return false;
	}

	struct VertexFormat {
		constexpr VertexFormat(const glm::vec2& p, const glm::vec2& t) : pos(p), tex(t) {}
		glm::vec2 pos;
		glm::vec2 tex;
	};
	alignas(16) constexpr VertexFormat vecs[] = {
		// left bottom
		VertexFormat(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f)),
		// right bottom
		VertexFormat(glm::vec2( 1.0f, -1.0f), glm::vec2(1.0f, 0.0f)),
		// right top
		VertexFormat(glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f)),
		// left bottom
		VertexFormat(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f)),
		// right top
		VertexFormat(glm::vec2( 1.0f,  1.0f), glm::vec2(1.0f)),
		// left top
		VertexFormat(glm::vec2(-1.0f,  1.0f), glm::vec2(0.0f, 1.0f)),
	};

	_postProcessBufId = _postProcessBuf.create(vecs, sizeof(vecs));
	_postProcessBuf.addAttribute(_postProcessShader.getPosAttribute(_postProcessBufId, &VertexFormat::pos));
	_postProcessBuf.addAttribute(_postProcessShader.getTexcoordAttribute(_postProcessBufId, &VertexFormat::tex));

	return true;
}

void WorldRenderer::initFrameBuffers(const glm::ivec2& dimensions) {
	video::TextureConfig textureCfg;
	textureCfg.wrap(video::TextureWrap::ClampToEdge);
	textureCfg.format(video::TextureFormat::RGB);
	video::FrameBufferConfig cfg;
	cfg.dimension(dimensions).depthBuffer(true).depthBufferFormat(video::TextureFormat::D24);
	cfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	_frameBuffer.init(cfg);

	textureCfg = video::TextureConfig();
	textureCfg.format(video::TextureFormat::RGB);
	video::FrameBufferConfig refractionCfg;
	refractionCfg.dimension(glm::ivec2(1280, 720)).depthBuffer(true).depthBufferFormat(video::TextureFormat::D32F);
	refractionCfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	_refractionBuffer.init(refractionCfg);

	textureCfg = video::TextureConfig();
	textureCfg.format(video::TextureFormat::RGB);
	video::FrameBufferConfig reflectionCfg;
	reflectionCfg.dimension(glm::ivec2(320, 180)).depthBuffer(true).depthBufferFormat(video::TextureFormat::D32F);
	reflectionCfg.addTextureAttachment(textureCfg, video::FrameBufferAttachment::Color0);
	_reflectionBuffer.init(reflectionCfg);
}

void WorldRenderer::shutdownFrameBuffers() {
	_frameBuffer.shutdown();
	_refractionBuffer.shutdown();
	_reflectionBuffer.shutdown();
}

void WorldRenderer::update(const video::Camera& camera, uint64_t dt) {
	core_trace_scoped(WorldRendererOnRunning);
	_focusPos = camera.target();
	_focusPos.y = 0.0f;//TODO: _world->findFloor(_focusPos.x, _focusPos.z, voxel::isFloor);

	_shadow.update(camera, _shadowMap->boolVal());

	_worldChunkMgr.update(_focusPos);
	_entityMgr.update(dt);
	_entityMgr.updateVisibleEntities(dt, camera);
}

}
