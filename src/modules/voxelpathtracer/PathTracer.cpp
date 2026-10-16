/**
 * @file
 */

#include "PathTracer.h"
#include "PathTracerScene.h"
#include "VoxelpathtracerBloomShader.h"
#include "VoxelpathtracerDisplayShader.h"
#include "VoxelpathtracerShader.h"
#include "VoxelpathtracerShaderConstants.h"
#include "color/ColorUtil.h"
#include "core/Common.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
#include "core/collection/Buffer.h"
#include "image/Image.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "video/Renderer.h"
#include "video/ScopedState.h"
#include "video/Shader.h"
#include "voxel/Region.h"
#include "voxelrender/RenderUtil.h"
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/vec4.hpp>

namespace voxelpathtracer {

namespace {

static constexpr int MaxSamplesThisPass = shader::VoxelpathtracerShaderConstants::getMaxSamplesThisPass();

static void shutdownTexture(video::TexturePtr &tex) {
	if (tex) {
		tex->shutdown();
		tex = nullptr;
	}
}

static video::TextureConfig nearestTextureConfig(video::TextureType type, video::TextureFormat format) {
	video::TextureConfig cfg;
	cfg.type(type);
	cfg.format(format);
	cfg.filter(video::TextureFilter::Nearest);
	cfg.wrap(video::TextureWrap::ClampToEdge);
	return cfg;
}

struct FullscreenPassState {
	video::ScopedState depthTest{video::State::DepthTest, false};
	video::ScopedState depthMask{video::State::DepthMask, false};
	video::ScopedState cullFace{video::State::CullFace, false};
	video::ScopedState blend{video::State::Blend, false};
};

static_assert((int)palette::MaterialType::Diffuse == shader::VoxelpathtracerShaderConstants::getMaterialDiffuse());
static_assert((int)palette::MaterialType::Metal == shader::VoxelpathtracerShaderConstants::getMaterialMetal());
static_assert((int)palette::MaterialType::Glass == shader::VoxelpathtracerShaderConstants::getMaterialGlass());
static_assert((int)palette::MaterialType::Emit == shader::VoxelpathtracerShaderConstants::getMaterialEmit());
static_assert((int)palette::MaterialType::Blend == shader::VoxelpathtracerShaderConstants::getMaterialBlend());
static_assert((int)palette::MaterialType::Media == shader::VoxelpathtracerShaderConstants::getMaterialMedia());

static glm::vec3 sunDirection(const PathTracerParams &params) {
	return voxelrender::sunDirectionFromAngle(glm::vec3(params.sunElevation, params.sunAzimuth, 0.0f));
}

static constexpr int MaterialRows = shader::VoxelpathtracerShaderConstants::getMaterialRows();
static_assert(sizeof(GpuEmitLight) == 8u * sizeof(float));

} // namespace

int PathTracer::maxTextureAxis() {
	const int brickSize = pathTracerBrickSize();
	int glMax = video::limiti(video::Limit::Max3DTextureSize);
	if (glMax < brickSize) {
		// GLES3 requires at least 256; desktop drivers typically report 2048.
		glMax = 2048;
	}
	return glMax;
}

PathTracer::~PathTracer() {
	stop();
	shutdown();
}

bool PathTracer::init() {
	return initGpu();
}

void PathTracer::shutdown() {
	shutdownGpu();
}

bool PathTracer::initGpu() {
	core_trace_scoped(PathTracerInitGpu);
	if (_initialized) {
		return true;
	}
	shader::VoxelpathtracerShader &traceShader = shader::VoxelpathtracerShader::getInstance();
	shader::VoxelpathtracerDisplayShader &displayShader = shader::VoxelpathtracerDisplayShader::getInstance();
	if (!traceShader.setup()) {
		Log::error("Failed to init pathtracer shader");
		return false;
	}
	if (!displayShader.setup()) {
		Log::error("Failed to init pathtracer display shader");
		traceShader.shutdown();
		return false;
	}

	if (!shader::VoxelpathtracerBloomShader::getInstance().setup()) {
		displayShader.shutdown();
		traceShader.shutdown();
		return false;
	}
	_bufferIndex = _vbo.createFullscreenQuad();
	_texIndex = _vbo.createFullscreenTextureBufferForFramebuffer();
	core_assert_always(_vbo.addAttribute(traceShader.getPosAttribute(_bufferIndex, &glm::vec2::x)));
	core_assert_always(_vbo.addAttribute(traceShader.getTexcoordAttribute(_texIndex, &glm::vec2::x)));
	core_memset(&_paramsUniforms, 0, sizeof(_paramsUniforms));
	core_memset(&_displayUniforms, 0, sizeof(_displayUniforms));
	shader::VoxelpathtracerBloomData::FilterData filter{};
	if (!_paramsUbo.create(_paramsUniforms) || !_displayUbo.create(_displayUniforms) || !_bloomUbo.create(filter)) {
		Log::error("Failed to create pathtracer uniform buffers");
		_paramsUbo.shutdown();
		_displayUbo.shutdown();
		_bloomUbo.shutdown();
		shader::VoxelpathtracerBloomShader::getInstance().shutdown();
		_vbo.shutdown();
		displayShader.shutdown();
		traceShader.shutdown();
		return false;
	}
	_initialized = true;
	return true;
}

void PathTracer::shutdownGpu() {
	if (!_initialized) {
		return;
	}
	_accum[0].shutdown();
	_accum[1].shutdown();
	_displayFbo.shutdown();
	_bloomFbo.shutdown();
	_bloomDirty = true;
	_displayDirty = true;
	shutdownTexture(_bricksTex);
	shutdownTexture(_indirectionTex);
	shutdownTexture(_paletteTex);
	shutdownTexture(_materialTex);
	shutdownTexture(_emitTex);
	_lightCount = 0;
	_emitterLuminance = 0.0f;
	_bricksPerRow = 1;
	_bricksPerCol = 1;
	_vbo.shutdown();
	_bufferIndex = -1;
	_texIndex = -1;
	_paramsUbo.shutdown();
	_displayUbo.shutdown();
	_bloomUbo.shutdown();
	shader::VoxelpathtracerBloomShader::getInstance().shutdown();
	shader::VoxelpathtracerShader::getInstance().shutdown();
	shader::VoxelpathtracerDisplayShader::getInstance().shutdown();
	_initialized = false;
	_targetResolution = 0;
}

int PathTracer::outputResolution() const {
	const float scale = glm::clamp(_state.params.renderScale, 0.25f, 1.0f);
	return core_max(1, (int)((float)core_max(1, _state.params.resolution) * scale + 0.5f));
}

void PathTracer::uploadEmitLights(const GpuEmitLight *lights, int count) {
	// u_lights RGBA32F, width=2, height=count: position and precomputed radiance.
	shutdownTexture(_emitTex);
	_lightCount = core_max(0, count);
	const int height = core_max(1, _lightCount);
	GpuEmitLight dummy;
	core_memset(&dummy, 0, sizeof(dummy));
	const GpuEmitLight *data = (lights != nullptr && _lightCount > 0) ? lights : &dummy;
	const video::TextureConfig cfg = nearestTextureConfig(video::TextureType::Texture2D, video::TextureFormat::RGBA32F);
	_emitTex = core::make_shared<video::Texture>(cfg, 2, height, "pathtracer-emit");
	_emitTex->upload(2, height, (const uint8_t *)data);
}

void PathTracer::drawFullscreen(video::FrameBuffer &target) {
	target.bind(false);
	core_assert_always(_vbo.bind());
	video::drawArrays(video::Primitive::Triangles, 6);
	_vbo.unbind();
	target.unbind();
}

bool PathTracer::ensureTargets(int resolution) {
	if (resolution < 1) {
		resolution = 1;
	}
	if (_targetResolution == resolution && _accum[0].handle() != video::InvalidId) {
		return true;
	}

	_accum[0].shutdown();
	_accum[1].shutdown();
	_displayFbo.shutdown();
	_bloomFbo.shutdown();
	_bloomDirty = true;
	_displayDirty = true;

	video::TextureConfig hdrCfg;
	hdrCfg.filter(video::TextureFilter::Nearest);
	hdrCfg.wrap(video::TextureWrap::ClampToEdge);

	video::TextureFormat hdrFormats[2] = {video::TextureFormat::RGBA32F, video::TextureFormat::RGBA16F};
	bool accumOk = false;
	for (int i = 0; i < 2; ++i) {
		_accum[0].shutdown();
		_accum[1].shutdown();
		hdrCfg.format(hdrFormats[i]);
		video::FrameBufferConfig accumCfg;
		accumCfg.dimension(glm::ivec2(resolution, resolution));
		accumCfg.addTextureAttachment(hdrCfg, video::FrameBufferAttachment::Color0);
		if (_accum[0].init(accumCfg) && _accum[1].init(accumCfg)) {
			accumOk = true;
			if (i == 1) {
				Log::warn("Pathtracer accumulation falling back to RGBA16F");
			}
			break;
		}
	}
	if (!accumOk) {
		Log::error("Failed to create pathtracer accumulation targets");
		return false;
	}

	video::FrameBufferConfig displayCfg;
	displayCfg.dimension(glm::ivec2(resolution, resolution));
	displayCfg.colorTexture(true);
	displayCfg.colorTextureFormat(video::TextureFormat::RGBA);
	if (!_displayFbo.init(displayCfg)) {
		Log::error("Failed to create pathtracer display target");
		return false;
	}

	_targetResolution = resolution;
	_writeAccum = 0;
	return true;
}

void PathTracer::setupTraceCamera(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera) {
	if (camera != nullptr) {
		_traceCamera = *camera;
	} else {
		_traceCamera.setSize(glm::ivec2(outputResolution()));
		_traceCamera.setMode(video::CameraMode::Perspective);
		_traceCamera.setType(video::CameraType::Free);
		const voxel::Region &sceneRegion = sceneGraph.sceneRegion();
		const glm::vec3 dim = glm::vec3(sceneRegion.getDimensionsInVoxels());
		const float farPlane = glm::max(32.0f, glm::length(dim) * 4.0f);
		voxelrender::configureCamera(_traceCamera, sceneRegion, voxelrender::SceneCameraMode::Free, farPlane);
	}
	_traceCamera.setSize(glm::ivec2(outputResolution()));
	_traceCamera.update(0.0);
}

bool PathTracer::createEmptyGpuScene() {
	_hasVolume = false;
	Log::debug("Pathtracer scene has no visible voxels");
	const int brickSize = pathTracerBrickSize();
	uint32_t emptyIndirection = 0;
	video::TextureConfig indCfg = nearestTextureConfig(video::TextureType::Texture3D, video::TextureFormat::R32U);
	indCfg.layers(1);
	_indirectionTex = core::make_shared<video::Texture>(indCfg, 1, 1, "pathtracer-indirection");
	_indirectionTex->upload((const uint8_t *)&emptyIndirection, 1);
	const size_t emptyBrickBytes = (size_t)brickSize * (size_t)brickSize * (size_t)brickSize * sizeof(uint32_t);
	uint32_t *emptyBricks = (uint32_t *)core_malloc(emptyBrickBytes);
	if (emptyBricks == nullptr) {
		Log::error("Failed to allocate empty pathtracer brick atlas");
		return false;
	}
	core_memset(emptyBricks, 0, emptyBrickBytes);
	video::TextureConfig brickCfg = indCfg;
	brickCfg.layers(brickSize);
	_bricksTex = core::make_shared<video::Texture>(brickCfg, brickSize, brickSize, "pathtracer-bricks");
	_bricksTex->upload((const uint8_t *)emptyBricks, brickSize);
	core_free(emptyBricks);
	_bricksPerRow = 1;
	_bricksPerCol = 1;
	const video::TextureConfig palCfg = nearestTextureConfig(video::TextureType::Texture2D, video::TextureFormat::RGBA);
	_paletteTex = core::make_shared<video::Texture>(palCfg, 1, 1, "pathtracer-palette");
	uint8_t pal[4] = {0, 0, 0, 0};
	_paletteTex->upload(1, 1, pal);
	video::TextureConfig matCfg = palCfg;
	matCfg.format(video::TextureFormat::RGBA32F);
	_materialTex = core::make_shared<video::Texture>(matCfg, MaterialRows, 1, "pathtracer-materials");
	GpuMaterial emptyMat;
	core_memset(&emptyMat, 0, sizeof(emptyMat));
	_materialTex->upload(MaterialRows, 1, (const uint8_t *)&emptyMat);
	_volumeMin = glm::ivec3(0);
	_volumeSize = glm::ivec3(1);
	uploadEmitLights(nullptr, 0);
	return true;
}

bool PathTracer::uploadSparseVolume(const SparseGpuVolume &volume, size_t materialCount) {
	const int brickSize = pathTracerBrickSize();
	const int voxelsPerBrick = pathTracerVoxelsPerBrick();
	const size_t atlasVoxels = (size_t)volume.atlasSize.x * (size_t)volume.atlasSize.y * (size_t)volume.atlasSize.z;
	uint32_t *atlas = (uint32_t *)core_malloc(core_max((size_t)1, atlasVoxels) * sizeof(uint32_t));
	if (atlas == nullptr) {
		Log::error("Failed to allocate pathtracer brick atlas");
		return false;
	}
	core_memset(atlas, 0, core_max((size_t)1, atlasVoxels) * sizeof(uint32_t));
	const int bricksPerCol = core_max(1, volume.bricksPerCol);
	for (int brickIndex = 0; brickIndex < volume.brickCount; ++brickIndex) {
		const int bx = brickIndex % volume.bricksPerRow;
		const int tmp = brickIndex / volume.bricksPerRow;
		const int by = tmp % bricksPerCol;
		const int bz = tmp / bricksPerCol;
		const uint32_t *src = volume.bricks + (size_t)brickIndex * (size_t)voxelsPerBrick;
		for (int lz = 0; lz < brickSize; ++lz) {
			for (int ly = 0; ly < brickSize; ++ly) {
				for (int lx = 0; lx < brickSize; ++lx) {
					const int ax = bx * brickSize + lx;
					const int ay = by * brickSize + ly;
					const int az = bz * brickSize + lz;
					atlas[volumeOffset(ax, ay, az, volume.atlasSize)] = src[(lz * brickSize + ly) * brickSize + lx];
				}
			}
		}
	}

	uint16_t *compactAtlas = compactGpuVoxelIndices(atlas, atlasVoxels, materialCount);
	const video::TextureFormat brickFormat =
		compactAtlas != nullptr ? video::TextureFormat::R16U : video::TextureFormat::R32U;
	video::TextureConfig brickCfg = nearestTextureConfig(video::TextureType::Texture3D, brickFormat);
	brickCfg.alignment(compactAtlas != nullptr ? 2 : 4);
	brickCfg.layers(volume.atlasSize.z);
	_bricksTex =
		core::make_shared<video::Texture>(brickCfg, volume.atlasSize.x, volume.atlasSize.y, "pathtracer-bricks");
	_bricksTex->upload(compactAtlas != nullptr ? (const uint8_t *)compactAtlas : (const uint8_t *)atlas,
					   volume.atlasSize.z);
	core_free(compactAtlas);
	core_free(atlas);

	const size_t coarseCount = (size_t)volume.coarseSize.x * (size_t)volume.coarseSize.y * (size_t)volume.coarseSize.z;
	const bool compactIndirection = volume.brickCount + (int)BrickIdBias <= (int)UINT16_MAX;
	video::TextureConfig indCfg = nearestTextureConfig(
		video::TextureType::Texture3D, compactIndirection ? video::TextureFormat::R16U : video::TextureFormat::R32U);
	indCfg.alignment(compactIndirection ? 2 : 4);
	indCfg.layers(volume.coarseSize.z);
	_indirectionTex =
		core::make_shared<video::Texture>(indCfg, volume.coarseSize.x, volume.coarseSize.y, "pathtracer-indirection");
	if (compactIndirection) {
		uint16_t *compact = (uint16_t *)core_malloc(coarseCount * sizeof(uint16_t));
		if (compact == nullptr) {
			Log::error("Failed to allocate pathtracer indirection upload");
			return false;
		}
		for (size_t i = 0; i < coarseCount; ++i) {
			compact[i] = (uint16_t)volume.indirection[i];
		}
		_indirectionTex->upload((const uint8_t *)compact, volume.coarseSize.z);
		core_free(compact);
	} else {
		_indirectionTex->upload((const uint8_t *)volume.indirection, volume.coarseSize.z);
	}
	_bricksPerRow = volume.bricksPerRow;
	_bricksPerCol = bricksPerCol;
	return true;
}

bool PathTracer::uploadMaterialAtlas(const GpuMaterialAtlas &atlas, const SparseGpuVolume &volume) {
	GpuEmitLight *emitLights = (GpuEmitLight *)core_malloc((size_t)MaxEmitLights * sizeof(GpuEmitLight));
	int emitCount = 0;
	if (emitLights != nullptr && atlas.materials != nullptr && atlas.albedoRgba != nullptr) {
		gatherSceneEmitLights(volume, atlas.materialCount, atlas.materials, atlas.albedoRgba, emitLights, emitCount,
							  _emitterLuminance);
		uploadEmitLights(emitLights, emitCount);
	} else {
		uploadEmitLights(nullptr, 0);
	}
	core_free(emitLights);

	shutdownTexture(_paletteTex);
	shutdownTexture(_materialTex);

	const int albedoWidth = core_max(1, atlas.materialCount);
	const video::TextureConfig palCfg = nearestTextureConfig(video::TextureType::Texture2D, video::TextureFormat::RGBA);
	// u_palette: flat RGBA8 albedo row, x = materialId - 1. See fetchPalette in shaders/_struct.glsl.
	_paletteTex = core::make_shared<video::Texture>(palCfg, albedoWidth, 1, "pathtracer-palette");
	if (atlas.albedoRgba != nullptr && atlas.materialCount > 0) {
		_paletteTex->upload(albedoWidth, 1, atlas.albedoRgba);
	} else {
		uint8_t empty[4] = {0, 0, 0, 0};
		_paletteTex->upload(1, 1, empty);
	}

	video::TextureConfig matCfg = nearestTextureConfig(video::TextureType::Texture2D, video::TextureFormat::RGBA32F);
	matCfg.alignment(4);
	const int materialHeight = core_max(1, atlas.materialCount);
	_materialTex = core::make_shared<video::Texture>(matCfg, MaterialRows, materialHeight, "pathtracer-materials");
	if (atlas.materials != nullptr && atlas.materialCount > 0) {
		_materialTex->upload(MaterialRows, materialHeight, (const uint8_t *)atlas.materials);
	} else {
		GpuMaterial emptyMat;
		core_memset(&emptyMat, 0, sizeof(emptyMat));
		_materialTex->upload(MaterialRows, 1, (const uint8_t *)&emptyMat);
	}
	return true;
}

bool PathTracer::uploadPreparedScene(PreparedGpuScene &prepared) {
	_state.cameraNames.clear();
	_state.volumeTruncated = prepared.volumeTruncated;
	_hasVolume = false;

	shutdownTexture(_bricksTex);
	shutdownTexture(_indirectionTex);
	shutdownTexture(_paletteTex);
	shutdownTexture(_materialTex);
	shutdownTexture(_emitTex);
	_lightCount = 0;
	_emitterLuminance = 0.0f;
	_bricksPerRow = 1;
	_bricksPerCol = 1;

	if (!prepared.success) {
		return false;
	}
	if (prepared.empty) {
		return createEmptyGpuScene();
	}

	if (!uploadSparseVolume(prepared.sparse, (size_t)prepared.materials.materialCount)) {
		return false;
	}
	if (!uploadMaterialAtlas(prepared.materials, prepared.sparse)) {
		return false;
	}

	_volumeMin = prepared.worldMin;
	_volumeSize = prepared.volumeSize;
	_hasVolume = true;
	_floorY = _state.params.floorEnabled ? _state.params.floorY : (float)prepared.worldMin.y;
	return true;
}

bool PathTracer::createScene(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera) {
	core_trace_scoped(PathTracerCreateScene);
	setupTraceCamera(sceneGraph, camera);
	PreparedGpuScene prepared = prepareGpuScene(sceneGraph, maxTextureAxis());
	return uploadPreparedScene(prepared);
}

void PathTracer::tracePass(int samples) {
	core_trace_scoped(PathTracerTracePass);
	shader::VoxelpathtracerShader &shader = shader::VoxelpathtracerShader::getInstance();
	video::ScopedShader scoped(shader);
	FullscreenPassState passState;

	const int readAccum = 1 - _writeAccum;
	core_assert_always(video::bindTexture(shader.getBoundBricksTexUnit(), _bricksTex));
	core_assert_always(video::bindTexture(shader.getBoundIndirectionTexUnit(), _indirectionTex));
	core_assert_always(video::bindTexture(shader.getBoundPaletteTexUnit(), _paletteTex));
	core_assert_always(video::bindTexture(shader.getBoundMaterialsTexUnit(), _materialTex));
	core_assert_always(video::bindTexture(shader.getBoundAccumulationTextureTexUnit(), _accum[readAccum].texture()));
	core_assert_always(video::bindTexture(shader.getBoundLightsTexUnit(), _emitTex));

	const PathTracerParams &params = _state.params;
	const glm::mat4 invViewProj = glm::inverse(_traceCamera.viewProjectionMatrix());
	const float elevScale = glm::clamp((_state.params.sunElevation + 6.0f) / 12.0f, 0.1f, 1.0f);
	const glm::vec3 sunColor = color::srgbToLinear(params.sunColor) * (params.sunIntensity * elevScale);
	const float sunCosAngle = glm::cos(glm::radians(glm::max(params.sunSoftness, 0.05f)));

	_paramsUniforms.invviewproj = invViewProj;
	for (int sample = 0; sample < samples; ++sample) {
		const glm::uvec2 point = cameraSobolPoint((uint32_t)(_state.currentSample + sample));
		const uint32_t seedHash = sampleSeedHash((uint32_t)(_state.currentSample + sample));
		// Split the integer hash into two exactly representable 16-bit float values.
		_paramsUniforms.camerasamples[sample] = glm::vec4((float)(point.x >> 8u), (float)(point.y >> 8u),
														  (float)(seedHash & 0xffffu), (float)(seedHash >> 16u));
	}
	_paramsUniforms.camerapos = _traceCamera.eye();
	_paramsUniforms.cameraright = _traceCamera.right();
	_paramsUniforms.cameraup = _traceCamera.up();
	_paramsUniforms.cameraforward = _traceCamera.forward();
	_paramsUniforms.sundir = sunDirection(params);
	_paramsUniforms.suncolor = sunColor;
	_paramsUniforms.skytop = color::srgbToLinear(params.skyTop) * params.skyIntensity;
	_paramsUniforms.skyhorizon = color::srgbToLinear(params.skyHorizon) * params.skyIntensity;
	_paramsUniforms.floorcolor = color::srgbToLinear(params.floorColor);
	const float skyPower = buildSkyDistribution(_paramsUniforms.skytop, _paramsUniforms.skyhorizon,
												_paramsUniforms.floorcolor * 0.25f, _paramsUniforms.skybands);
	const float sunPower = glm::dot(sunColor, glm::vec3(0.2126f, 0.7152f, 0.0722f));
	_paramsUniforms.skySamplingProbability = skyPower > 0.0f ? skyPower / (skyPower + sunPower) : 0.0f;
	_paramsUniforms.inverseEmitterLuminance = _emitterLuminance > 0.0f ? 1.0f / _emitterLuminance : 0.0f;
	_paramsUniforms.bgcolor = color::srgbToLinear(params.backgroundColor);
	_paramsUniforms.fogcolor = color::srgbToLinear(params.fogColor);
	_paramsUniforms.resolution = glm::vec2((float)_targetResolution, (float)_targetResolution);
	_paramsUniforms.suncosangle = sunCosAngle;
	_paramsUniforms.floory = _floorY;
	_paramsUniforms.fogdensity = params.fogDensity;
	_paramsUniforms.aperture = params.depthOfField ? params.aperture : 0.0f;
	_paramsUniforms.focusdistance = params.focusDistance > 0.0f ? params.focusDistance : _traceCamera.targetDistance();
	_paramsUniforms.volumeactive = _hasVolume ? 1 : 0;
	_paramsUniforms.ortho = _traceCamera.isOrthographic() ? 1 : 0;
	_paramsUniforms.frame = _state.currentSample;
	_paramsUniforms.samplesPerPass = samples;
	_paramsUniforms.floorenabled = params.floorEnabled ? 1 : 0;
	_paramsUniforms.bgmode = params.background;
	_paramsUniforms.bounces = params.bounces;
	_paramsUniforms.edgeradius = glm::clamp(params.voxelEdge, 0.0f, 1.0f);
	_paramsUniforms.atmosphere = glm::max(params.atmosphere, 0.0f);
	_paramsUniforms.lightcount = _lightCount;
	_paramsUniforms.volumemin = _volumeMin;
	_paramsUniforms.volumesize = _volumeSize;
	_paramsUniforms.bricksperrow = _bricksPerRow;
	_paramsUniforms.brickspercol = _bricksPerCol;
	core_assert_always(_paramsUbo.update(_paramsUniforms));
	core_assert_always(shader.setParams(_paramsUbo.getParamsUniformBuffer()));
	drawFullscreen(_accum[_writeAccum]);

	_state.currentSample += samples;
	_writeAccum = readAccum;
	_displayDirty = true;
	_bloomDirty = true;
}

bool PathTracer::bloomPass(float exposure) {
	if (_bloomFbo.handle() == video::InvalidId) {
		video::FrameBufferConfig config;
		config.dimension(glm::ivec2(_targetResolution));
		config.colorTexture(true);
		config.colorTextureFormat(video::TextureFormat::RGBA32F);
		if (!_bloomFbo.init(config)) {
			return false;
		}
		_bloomDirty = true;
	}
	if (!_bloomDirty) {
		return true;
	}
	shader::VoxelpathtracerBloomShader &shader = shader::VoxelpathtracerBloomShader::getInstance();
	video::ScopedShader scoped(shader);
	FullscreenPassState passState;
	core_assert_always(video::bindTexture(shader.getBoundTextureTexUnit(), _accum[1 - _writeAccum].texture()));
	shader::VoxelpathtracerBloomData::FilterData filter{};
	filter.resolution = glm::vec2((float)_targetResolution);
	filter.exposure = exposure;
	core_assert_always(_bloomUbo.update(filter));
	core_assert_always(shader.setFilter(_bloomUbo.getFilterUniformBuffer()));
	drawFullscreen(_bloomFbo);
	_bloomDirty = false;
	return true;
}

void PathTracer::displayPass() {
	core_trace_scoped(PathTracerDisplayPass);
	shader::VoxelpathtracerDisplayData::DisplayData nextDisplay{};
	const PathTracerParams &params = _state.params;
	nextDisplay.screensize = glm::vec2((float)_targetResolution, (float)_targetResolution);
	nextDisplay.exposure = params.exposure;
	nextDisplay.contrast = params.contrast;
	nextDisplay.saturation = params.saturation;
	nextDisplay.vignette = params.vignette;
	nextDisplay.tonemapmode = params.toneMapping;
	nextDisplay.checkerboard = params.background == PathTracerBackgroundTransparent ? 1 : 0;
	nextDisplay.bloomstrength = glm::clamp(params.bloom, 0.0f, 2.0f);
	if (!_displayDirty && nextDisplay.screensize == _displayUniforms.screensize &&
		nextDisplay.exposure == _displayUniforms.exposure && nextDisplay.contrast == _displayUniforms.contrast &&
		nextDisplay.saturation == _displayUniforms.saturation && nextDisplay.vignette == _displayUniforms.vignette &&
		nextDisplay.tonemapmode == _displayUniforms.tonemapmode &&
		nextDisplay.checkerboard == _displayUniforms.checkerboard &&
		nextDisplay.bloomstrength == _displayUniforms.bloomstrength) {
		return;
	}
	_bloomDirty |= nextDisplay.exposure != _displayUniforms.exposure;
	nextDisplay.bloomprecomputed = nextDisplay.bloomstrength > 1e-4f && bloomPass(nextDisplay.exposure) ? 1 : 0;
	_displayUniforms = nextDisplay;
	shader::VoxelpathtracerDisplayShader &shader = shader::VoxelpathtracerDisplayShader::getInstance();
	video::ScopedShader scoped(shader);
	FullscreenPassState passState;
	const int readAccum = 1 - _writeAccum;
	core_assert_always(video::bindTexture(shader.getBoundTextureTexUnit(), _accum[readAccum].texture()));
	core_assert_always(video::bindTexture(shader.getBoundBloomTextureTexUnit(), _displayUniforms.bloomprecomputed != 0
																					? _bloomFbo.texture()
																					: _accum[readAccum].texture()));
	core_assert_always(_displayUbo.update(_displayUniforms));
	core_assert_always(shader.setDisplay(_displayUbo.getDisplayUniformBuffer()));
	drawFullscreen(_displayFbo);
	_displayDirty = false;
}

void PathTracer::resetAccumulation() {
	if (!initGpu()) {
		return;
	}
	if (!ensureTargets(outputResolution())) {
		return;
	}
	_state.currentSample = 0;
	_writeAccum = 0;
	_state.started = true;
}

bool PathTracer::cameraPoseChanged(const video::Camera &camera) const {
	if (!glm::equal(camera.viewProjectionMatrix(), _traceCamera.viewProjectionMatrix())) {
		return true;
	}
	return camera.isOrthographic() != _traceCamera.isOrthographic();
}

void PathTracer::applyCamera(const video::Camera *camera) {
	if (camera == nullptr) {
		return;
	}
	video::Camera cam = *camera;
	const int resolution = outputResolution();
	cam.setSize(glm::ivec2(resolution));
	cam.update(0.0);
	const bool changed = cameraPoseChanged(cam);
	_traceCamera = cam;
	if (changed) {
		resetAccumulation();
	}
}

bool PathTracer::start(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera) {
	if (!initGpu()) {
		return false;
	}
	Log::debug("Create pathtracer scene");
	if (!createScene(sceneGraph, camera)) {
		return false;
	}
	if (!ensureTargets(outputResolution())) {
		return false;
	}
	_state.currentSample = 0;
	_writeAccum = 0;
	_state.started = true;
	Log::debug("Started pathtracer");
	return true;
}

bool PathTracer::startPrepared(PreparedGpuScene &&prepared, const scenegraph::SceneGraph &sceneGraph,
							   const video::Camera *camera) {
	if (!initGpu()) {
		return false;
	}
	Log::debug("Upload prepared pathtracer scene");
	setupTraceCamera(sceneGraph, camera);
	if (!uploadPreparedScene(prepared)) {
		return false;
	}
	prepared.free();
	if (!ensureTargets(outputResolution())) {
		return false;
	}
	_state.currentSample = 0;
	_writeAccum = 0;
	_state.started = true;
	Log::debug("Started pathtracer from prepared scene");
	return true;
}

bool PathTracer::restart(const scenegraph::SceneGraph &sceneGraph, const video::Camera *camera) {
	Log::debug("Restart pathtracer");
	stop();
	return start(sceneGraph, camera);
}

bool PathTracer::stop() {
	_state.started = false;
	return true;
}

bool PathTracer::resume() {
	if (!_initialized) {
		return false;
	}
	_state.started = true;
	return true;
}

bool PathTracer::started() const {
	return _state.started;
}

bool PathTracer::hasVolume() const {
	return _hasVolume;
}

bool PathTracer::update(int *currentSample) {
	if (!_state.started) {
		if (currentSample != nullptr) {
			*currentSample = _state.currentSample;
		}
		return true;
	}
	if (_state.currentSample >= _state.params.samples) {
		_state.started = false;
		if (currentSample != nullptr) {
			*currentSample = _state.currentSample;
		}
		return true;
	}

	const int remaining = _state.params.samples - _state.currentSample;
	const int samples = core_min(remaining, core_max(1, core_min(MaxSamplesThisPass, _state.params.samplesThisPass)));
	tracePass(samples);
	if (currentSample != nullptr) {
		*currentSample = _state.currentSample;
	}
	if (_state.currentSample >= _state.params.samples) {
		_state.started = false;
		return true;
	}
	return false;
}

video::TexturePtr PathTracer::texture() {
	if (_displayFbo.handle() == video::InvalidId) {
		return video::TexturePtr();
	}
	displayPass();
	return _displayFbo.texture();
}

image::ImagePtr PathTracer::image() {
	if (!texture()) {
		return image::ImagePtr();
	}
	return _displayFbo.image("pathtracer");
}

} // namespace voxelpathtracer
