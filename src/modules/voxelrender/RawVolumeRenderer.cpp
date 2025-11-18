/**
 * @file
 */

#include "RawVolumeRenderer.h"
#include "ShaderAttribute.h"
#include "VoxelShaderConstants.h"
#include "app/App.h"
#include "app/Async.h"
#include "core/Algorithm.h"
#include "core/ArrayLength.h"
#include "core/Color.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
#include "core/Var.h"
#include "palette/NormalPalette.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "video/FrameBufferConfig.h"
#include "video/Renderer.h"
#include "video/ScopedFaceCull.h"
#include "video/ScopedFrameBuffer.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "video/TextureConfig.h"
#include "video/Types.h"
#include "voxel/MaterialColor.h"
#include "voxel/MeshState.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelutil/VolumeVisitor.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/epsilon.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/norm.hpp>

namespace voxelrender {

bool RenderContext::isEditMode() const {
	return renderMode == RenderMode::Edit;
}

bool RenderContext::isSceneMode() const {
	return renderMode == RenderMode::Scene;
}

bool RenderContext::applyTransforms() const {
	return isSceneMode() || applyTransformsInEditMode;
}

bool RenderContext::showCameras() const {
	return isSceneMode();
}

bool RenderContext::init(const glm::ivec2 &size) {
	video::FrameBufferConfig cfg;
	cfg.dimension(size);

	// Configure multisampling based on configuration variables
	const core::VarPtr &multisampleSamplesVar = core::Var::getSafe(cfg::ClientMultiSampleSamples);
	const core::VarPtr &multisampleBuffersVar = core::Var::getSafe(cfg::ClientMultiSampleBuffers);
	enableMultisampling = multisampleSamplesVar->intVal() > 0 && multisampleBuffersVar->intVal() > 0;
	multisampleSamples = multisampleSamplesVar->intVal();

	if (enableMultisampling && multisampleSamples > 1) {
		// Clamp to supported range
		const int maxSamples = video::limiti(video::Limit::MaxSamples);
		Log::debug("Hardware supports up to %d multisampling samples, requested: %d", maxSamples, multisampleSamples);
		multisampleSamples = glm::clamp(multisampleSamples, 2, maxSamples);

		// Ensure it's a power of 2 (common requirement)
		if ((multisampleSamples & (multisampleSamples - 1)) != 0) {
			// Find the next lower power of 2
			int powerOf2 = 2;
			while (powerOf2 * 2 <= multisampleSamples) {
				powerOf2 *= 2;
			}
			multisampleSamples = powerOf2;
			Log::debug("Adjusted to power of 2: %d samples", multisampleSamples);
		}

		Log::debug("Initializing volume renderer framebuffer with %d multisampling samples", multisampleSamples);
	} else {
		enableMultisampling = false;
		multisampleSamples = 0;
	}
	// Configure multisampling for the entire framebuffer (affects depth buffer)
	if (enableMultisampling) {
		cfg.samples(multisampleSamples);
	}

	// Add texture attachments
	if (enableMultisampling) {
		video::TextureConfig msaaConfig = video::createDefaultMultiSampleTextureConfig();
		msaaConfig.samples(multisampleSamples);  // Set the actual sample count
		Log::debug("MSAA texture config - type: %d, samples: %d, format: %d",
			(int)msaaConfig.type(), msaaConfig.samples(), (int)msaaConfig.format());
		cfg.addTextureAttachment(msaaConfig, video::FrameBufferAttachment::Color0); // scene
		cfg.addTextureAttachment(msaaConfig, video::FrameBufferAttachment::Color1); // bloom (also MSAA for consistency)

		// Add multisampled depth buffer
		video::TextureConfig msaaDepthConfig = video::createDefaultMultiSampleTextureConfig();
		msaaDepthConfig.samples(multisampleSamples);
		msaaDepthConfig.format(video::TextureFormat::D24S8);
		cfg.addTextureAttachment(msaaDepthConfig, video::FrameBufferAttachment::DepthStencil);
	} else {
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
		cfg.depthBuffer(true);
	}

	if (!frameBuffer.init(cfg)) {
		if (enableMultisampling) {
			Log::warn("Failed to initialize multisampled framebuffer, retrying without multisampling");
			// Retry without multisampling - first shutdown the failed framebuffer
			frameBuffer.shutdown();
			enableMultisampling = false;
			multisampleSamples = 0;
			video::FrameBufferConfig fallbackCfg;
			fallbackCfg.dimension(size);
			fallbackCfg.samples(0); // Explicitly disable multisampling
			fallbackCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0);
			fallbackCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1);
			fallbackCfg.depthBuffer(true);
			if (!frameBuffer.init(fallbackCfg)) {
				Log::error("Failed to initialize the volume renderer framebuffer");
				return false;
			}
		} else {
			Log::error("Failed to initialize the volume renderer framebuffer");
			return false;
		}
	}

	// If multisampling is enabled, create a resolve framebuffer with regular textures
	if (enableMultisampling) {
		video::FrameBufferConfig resolveCfg;
		resolveCfg.dimension(size);
		resolveCfg.samples(0); // No multisampling for resolve target
		resolveCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0);
		resolveCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1);
		resolveCfg.depthBuffer(true);

		if (!resolveFrameBuffer.init(resolveCfg)) {
			Log::error("Failed to initialize resolve framebuffer for multisampling");
			return false;
		}
		Log::debug("Successfully created resolve framebuffer for multisampling");
	}

	// we have to do an y-flip here due to the framebuffer handling
	if (!bloomRenderer.init(true, size.x, size.y)) {
		Log::error("Failed to initialize the bloom renderer");
		return false;
	}
	return true;
}

bool RenderContext::resize(const glm::ivec2 &size) {
	if (frameBuffer.dimension() == size) {
		return true;
	}
	frameBuffer.shutdown();
	video::FrameBufferConfig cfg;
	cfg.dimension(size);

	// Configure multisampling for the entire framebuffer (affects depth buffer)
	if (enableMultisampling) {
		cfg.samples(multisampleSamples);
	}

	// Add texture attachments
	if (enableMultisampling) {
		video::TextureConfig msaaConfig = video::createDefaultMultiSampleTextureConfig();
		msaaConfig.samples(multisampleSamples);  // Set the actual sample count
		Log::info("MSAA resize: texture config - type: %d, samples: %d, format: %d",
			(int)msaaConfig.type(), msaaConfig.samples(), (int)msaaConfig.format());
		cfg.addTextureAttachment(msaaConfig, video::FrameBufferAttachment::Color0); // scene
		cfg.addTextureAttachment(msaaConfig, video::FrameBufferAttachment::Color1); // bloom (also MSAA for consistency)

		// Add multisampled depth buffer
		video::TextureConfig msaaDepthConfig = video::createDefaultMultiSampleTextureConfig();
		msaaDepthConfig.samples(multisampleSamples);
		msaaDepthConfig.format(video::TextureFormat::D24S8);
		cfg.addTextureAttachment(msaaDepthConfig, video::FrameBufferAttachment::DepthStencil);
	} else {
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
		cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
		cfg.depthBuffer(true);
	}
	// Check GL state before framebuffer creation
	if (enableMultisampling) {
		int maxSamples = video::limiti(video::Limit::MaxSamples);
		Log::debug("Resize GL_MAX_SAMPLES: %d, requested: %d", maxSamples, multisampleSamples);
	}

	if (!frameBuffer.init(cfg)) {
		Log::error("Failed to initialize the volume renderer framebuffer - FB incomplete multisample detected");
		return false;
	}
	Log::debug("Successfully created %s framebuffer in resize", enableMultisampling ? "multisampled" : "regular");

	// If multisampling is enabled, create/resize resolve framebuffer with regular textures
	if (enableMultisampling) {
		resolveFrameBuffer.shutdown();
		video::FrameBufferConfig resolveCfg;
		resolveCfg.dimension(size);
		resolveCfg.samples(0); // No multisampling for resolve target
		resolveCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0);
		resolveCfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1);
		resolveCfg.depthBuffer(true);

		if (!resolveFrameBuffer.init(resolveCfg)) {
			Log::error("Failed to initialize resolve framebuffer for multisampling");
			return false;
		}
		Log::debug("Successfully created resolve framebuffer in resize");
	}

	// we have to do an y-flip here due to the framebuffer handling
	if (!bloomRenderer.resize(size.x, size.y)) {
		Log::error("Failed to initialize the bloom renderer");
		return false;
	}
	return true;
}

bool RenderContext::updateMultisampling() {
	const core::VarPtr &multisampleSamplesVar = core::Var::getSafe(cfg::ClientMultiSampleSamples);
	const core::VarPtr &multisampleBuffersVar = core::Var::getSafe(cfg::ClientMultiSampleBuffers);
	bool newEnableMultisampling = multisampleSamplesVar->intVal() > 0 && multisampleBuffersVar->intVal() > 0;
	int newMultisampleSamples = multisampleSamplesVar->intVal();

	if (enableMultisampling != newEnableMultisampling || multisampleSamples != newMultisampleSamples) {
		enableMultisampling = newEnableMultisampling;
		multisampleSamples = newMultisampleSamples;
		// Recreate the framebuffer with new multisampling settings
		const glm::ivec2 currentSize = frameBuffer.dimension();
		return resize(currentSize);
	}
	return true;
}

void RenderContext::shutdown() {
	frameBuffer.shutdown();
	resolveFrameBuffer.shutdown();
	bloomRenderer.shutdown();
}

RawVolumeRenderer::RawVolumeRenderer()
	: _voxelShader(shader::VoxelShader::getInstance()), _voxelNormShader(shader::VoxelnormShader::getInstance()),
	  _shadowMapShader(shader::ShadowmapShader::getInstance()) {
}

void RawVolumeRenderer::construct() {
}

bool RawVolumeRenderer::initStateBuffers(bool normals) {
	for (int idx = 0; idx < voxel::MAX_VOLUMES; ++idx) {
		RenderState &state = _state[idx];
		for (int i = 0; i < voxel::MeshType_Max; ++i) {
			state._vertexBufferIndex[i] = state._vertexBuffer[i].create();
			if (state._vertexBufferIndex[i] == -1) {
				Log::error("Could not create the vertex buffer object");
				return false;
			}

			if (normals) {
				state._normalBufferIndex[i] = state._vertexBuffer[i].create();
				if (state._normalBufferIndex[i] == -1) {
					Log::error("Could not create the normal buffer object");
					return false;
				}
			}

			state._indexBufferIndex[i] = state._vertexBuffer[i].create(nullptr, 0, video::BufferType::IndexBuffer);
			if (state._indexBufferIndex[i] == -1) {
				Log::error("Could not create the vertex buffer object for the indices");
				return false;
			}
			if (i == voxel::MeshType_Transparency) {
				// we are sorting the transparency buffer every frame
				state._vertexBuffer[i].setMode(state._indexBufferIndex[i], video::BufferMode::Dynamic);
			}
		}
	}

	if (normals) {
		for (int idx = 0; idx < voxel::MAX_VOLUMES; ++idx) {
			RenderState &state = _state[idx];
			for (int i = 0; i < voxel::MeshType_Max; ++i) {
				const video::Attribute &attributePos =
					getPositionVertexAttribute(state._vertexBufferIndex[i], _voxelNormShader.getLocationPos(),
											   _voxelNormShader.getComponentsPos());
				state._vertexBuffer[i].addAttribute(attributePos);

				const video::Attribute &attributeInfo =
					getInfoVertexAttribute(state._vertexBufferIndex[i], _voxelNormShader.getLocationInfo(),
										   _voxelNormShader.getComponentsInfo());
				state._vertexBuffer[i].addAttribute(attributeInfo);

				const video::Attribute &attributeNormal =
					getNormalVertexAttribute(state._normalBufferIndex[i], _voxelNormShader.getLocationNormal(),
											 _voxelNormShader.getComponentsNormal());
				state._vertexBuffer[i].addAttribute(attributeNormal);
			}
		}
	} else {
		for (int idx = 0; idx < voxel::MAX_VOLUMES; ++idx) {
			RenderState &state = _state[idx];
			for (int i = 0; i < voxel::MeshType_Max; ++i) {
				const video::Attribute &attributePos = getPositionVertexAttribute(
					state._vertexBufferIndex[i], _voxelShader.getLocationPos(), _voxelShader.getComponentsPos());
				state._vertexBuffer[i].addAttribute(attributePos);

				const video::Attribute &attributeInfo = getInfoVertexAttribute(
					state._vertexBufferIndex[i], _voxelShader.getLocationInfo(), _voxelShader.getComponentsInfo());
				state._vertexBuffer[i].addAttribute(attributeInfo);

				const video::Attribute &attributeInfo2 =
					getInfo2VertexAttribute(state._vertexBufferIndex[i], _voxelShader.getLocationInfo2(),
										   _voxelShader.getComponentsInfo2());
				state._vertexBuffer[i].addAttribute(attributeInfo2);
			}
		}
	}

	return true;
}

bool RawVolumeRenderer::init(bool normals) {
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_bloom = core::Var::getSafe(cfg::ClientBloom);
	_cullBuffers = core::Var::getSafe(cfg::RenderCullBuffers);
	_cullNodes = core::Var::getSafe(cfg::RenderCullNodes);

	if (!_voxelShader.setup()) {
		Log::error("Failed to initialize the voxel shader");
		return false;
	}
	if (!_voxelNormShader.setup()) {
		Log::error("Failed to initialize the voxelnorm shader");
		return false;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return false;
	}
	alignas(16) shader::ShadowmapData::BlockData var;
	_shadowMapUniformBlock.create(var);

	if (!initStateBuffers(normals)) {
		Log::error("Failed to initialize the state buffers");
		return false;
	}

	const int shaderMaterialColorsArraySize = lengthof(shader::VoxelData::VertData::materialcolor);
	if constexpr (shaderMaterialColorsArraySize != palette::PaletteMaxColors) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				   shaderMaterialColorsArraySize, palette::PaletteMaxColors);
		return false;
	}

	if (_voxelShader.getLocationPos() != _voxelNormShader.getLocationPos()) {
		Log::error("Shader attribute order doesn't match for pos (%i/%i)", _voxelShader.getLocationPos(),
				   _voxelNormShader.getLocationPos());
		return false;
	}

	if (_voxelShader.getLocationInfo() != _voxelNormShader.getLocationInfo()) {
		Log::error("Shader attribute order doesn't match for info (%i/%i)", _voxelShader.getLocationInfo(),
				   _voxelNormShader.getLocationInfo());
		return false;
	}

	voxelrender::ShadowParameters shadowParams;
	shadowParams.maxDepthBuffers = shader::VoxelShaderConstants::getMaxDepthBuffers();
	if (!_shadow.init(shadowParams)) {
		Log::error("Failed to initialize the shadow object");
		return false;
	}

	_voxelShaderFragData.diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
	_voxelShaderFragData.ambientColor = glm::vec3(1.0f, 1.0f, 1.0f);

	_voxelData.create(_voxelShaderFragData);
	_voxelData.create(_voxelShaderVertData);

	_shapeRenderer.init();

	return true;
}

void RawVolumeRenderer::scheduleRegionExtraction(const voxel::MeshStatePtr &meshState, int idx, const voxel::Region &region) {
	if (meshState->scheduleRegionExtraction(idx, region)) {
		deleteMeshes(idx);
	}
}

void RawVolumeRenderer::update(const voxel::MeshStatePtr &meshState) {
	core_trace_scoped(RawVolumeRendererUpdateMeshState);
	if (meshState->update()) {
		resetStateBuffers(meshState->hasNormals());
	}

	int cnt = 0;

	const core::TimeProviderPtr& timeProvider = app::App::getInstance()->timeProvider();
	const uint64_t startTime = timeProvider->systemMillis();
	for (;;) {
		const int idx = meshState->pop();
		if (idx == -1) {
			break;
		}
		if (!updateBufferForVolume(meshState, idx)) {
			Log::error("Failed to update the mesh at index %i", idx);
		}
		++cnt;
		const uint64_t deltaT = timeProvider->systemMillis() - startTime;
		if (deltaT > 50u) { // 50ms max
			Log::debug("Update took too long: %u ms", (int32_t)(deltaT));
			break;
		}
	}
	if (cnt > 0) {
		Log::debug("Perform %i mesh updates in this frame", cnt);
	}
}

bool RawVolumeRenderer::updateIndexBufferForVolumeCull(const voxel::MeshStatePtr &meshState,
													   const video::Camera &camera, int idx, voxel::MeshType type,
													   size_t indCount) {
	core_trace_scoped(UpdateIndexBufferForVolumeCull);
	const size_t indicesBufSize = indCount * sizeof(voxel::IndexType);
	voxel::IndexType *indicesBuf = (voxel::IndexType *)core_malloc(indicesBufSize);
	voxel::IndexType *indicesPos = indicesBuf;
	voxel::IndexType offset = (voxel::IndexType)0;
	const int bufferIndex = meshState->resolveIdx(idx);
	RenderState &state = _state[bufferIndex];
	const voxel::MeshState::MeshesMap & meshesMap = meshState->meshes(type);
	core::VarPtr meshSize = core::Var::getSafe(cfg::VoxelMeshSize);
	int culled = 0;
	for (const auto &i : meshesMap) {
		const voxel::MeshState::Meshes &meshes = i->second;
		const voxel::Mesh *mesh = meshes[bufferIndex];
		if (mesh == nullptr || mesh->getNoOfIndices() <= 0) {
			continue;
		}
		const glm::ivec3 &mins = i->first;
		const glm::ivec3 &maxs = mins + (meshSize->intVal() - 1);
		const glm::vec3 &mi = meshState->model(idx) * glm::vec4(mins, 1.0f);
		const glm::vec3 &ma = meshState->model(idx) * glm::vec4(maxs, 1.0f);
		if (!camera.isVisible(mi, ma)) {
			offset += mesh->getNoOfVertices();
			++culled;
			Log::trace("Culled mesh at index %i at pos %i:%i:%i", idx, mins.x, mins.y, mins.z);
			continue;
		}
		const voxel::IndexArray &indexVector = mesh->getIndexVector();
		core_memcpy(indicesPos, &indexVector[0], indexVector.size() * sizeof(voxel::IndexType));
		for (size_t j = 0; j < indexVector.size(); ++j) {
			*indicesPos++ += offset;
		}
		offset += mesh->getNoOfVertices();
	}
	Log::debug("update indexbuffer with culling: %i (type: %i, culled: %i)", idx, type, culled);
	if (!state._vertexBuffer[type].update(state._indexBufferIndex[type], indicesBuf, indicesBufSize, true)) {
		Log::error("Failed to update the index buffer with culling");
		core_free(indicesBuf);
		return false;
	}
	core_free(indicesBuf);
	return true;
}

bool RawVolumeRenderer::updateIndexBufferForVolume(const voxel::MeshStatePtr &meshState, int idx, voxel::MeshType type, size_t indCount) {
	core_trace_scoped(UpdateIndexBufferForVolume);
	const size_t indicesBufSize = indCount * sizeof(voxel::IndexType);
	voxel::IndexType *indicesBuf = (voxel::IndexType *)core_malloc(indicesBufSize);
	voxel::IndexType *indicesPos = indicesBuf;
	voxel::IndexType offset = (voxel::IndexType)0;
	const int bufferIndex = meshState->resolveIdx(idx);
	RenderState &state = _state[bufferIndex];
	for (const auto &i : meshState->meshes(type)) {
		const voxel::MeshState::Meshes &meshes = i->second;
		const voxel::Mesh *mesh = meshes[bufferIndex];
		if (mesh == nullptr || mesh->getNoOfIndices() <= 0) {
			continue;
		}
		const voxel::IndexArray &indexVector = mesh->getIndexVector();
		core_memcpy(indicesPos, &indexVector[0], indexVector.size() * sizeof(voxel::IndexType));
		for (size_t j = 0; j < indexVector.size(); ++j) {
			*indicesPos++ += offset;
		}
		offset += mesh->getNoOfVertices();
	}
	Log::debug("update indexbuffer: %i (type: %i)", idx, type);
	if (!state._vertexBuffer[type].update(state._indexBufferIndex[type], indicesBuf, indicesBufSize, true)) {
		Log::error("Failed to update the index buffer");
		core_free(indicesBuf);
		return false;
	}
	core_free(indicesBuf);
	return true;
}

bool RawVolumeRenderer::updateBufferForVolume(const voxel::MeshStatePtr &meshState, int idx, voxel::MeshType type, UpdateBufferFlags flags) {
	if (idx < 0 || idx >= voxel::MAX_VOLUMES) {
		return false;
	}
	core_trace_scoped(RawVolumeRendererUpdate);

	const int bufferIndex = meshState->resolveIdx(idx);
	size_t vertCount = 0u;
	size_t normalsCount = 0u;
	size_t indCount = 0u;
	meshState->count(type, bufferIndex, vertCount, normalsCount, indCount);

	RenderState &state = _state[bufferIndex];
	if (indCount == 0u || vertCount == 0u) {
		core_trace_scoped(ClearVertexBuffer);
		Log::debug("clear vertexbuffer: %i", idx);
		video::Buffer &buffer = state._vertexBuffer[type];
		buffer.update(state._vertexBufferIndex[type], nullptr, 0);
		buffer.update(state._normalBufferIndex[type], nullptr, 0);
		buffer.update(state._indexBufferIndex[type], nullptr, 0);
		state._dirtyNormals = true;
		return true;
	}

	// TODO: PERF: maybe we could cull here by updating the index buffer only to leave out the mesh instances that are not visible
	//             right now only the whole RenderState is culled or not - but for huge volumes, it might make a big difference to
	//             cull individual mesh instances here

	if (flags == UpdateBufferFlags::Indices) {
		return updateIndexBufferForVolume(meshState, idx, type, indCount);
	}

	const size_t verticesBufSize = vertCount * sizeof(voxel::VoxelVertex);
	voxel::VoxelVertex *verticesBuf = (voxel::VoxelVertex *)core_malloc(verticesBufSize);
	const size_t normalsBufSize = normalsCount * sizeof(glm::vec3);
	glm::vec3 *normalsBuf = (glm::vec3 *)core_malloc(normalsBufSize);
	const size_t indicesBufSize = indCount * sizeof(voxel::IndexType);
	voxel::IndexType *indicesBuf = (voxel::IndexType *)core_malloc(indicesBufSize);

	voxel::VoxelVertex *verticesPos = verticesBuf;
	glm::vec3 *normalsPos = normalsBuf;
	voxel::IndexType *indicesPos = indicesBuf;

	voxel::IndexType offset = (voxel::IndexType)0;
	for (const auto &i : meshState->meshes(type)) {
		const voxel::MeshState::Meshes &meshes = i->second;
		const voxel::Mesh *mesh = meshes[bufferIndex];
		if (mesh == nullptr || mesh->getNoOfIndices() <= 0) {
			continue;
		}
		const voxel::VertexArray &vertexVector = mesh->getVertexVector();
		const voxel::NormalArray &normalVector = mesh->getNormalVector();
		const voxel::IndexArray &indexVector = mesh->getIndexVector();
		core_memcpy(verticesPos, &vertexVector[0], vertexVector.size() * sizeof(voxel::VoxelVertex));
		if (!normalVector.empty()) {
			core_assert(vertexVector.size() == normalVector.size());
			core_memcpy(normalsPos, &normalVector[0], normalVector.size() * sizeof(glm::vec3));
		}
		core_memcpy(indicesPos, &indexVector[0], indexVector.size() * sizeof(voxel::IndexType));

		for (size_t j = 0; j < indexVector.size(); ++j) {
			*indicesPos++ += offset;
		}

		verticesPos += vertexVector.size();
		normalsPos += normalVector.size();
		offset += vertexVector.size();
	}
	state._dirtyNormals = true;

	Log::debug("update vertexbuffer: %i (type: %i)", idx, type);
	if (!state._vertexBuffer[type].update(state._vertexBufferIndex[type], verticesBuf, verticesBufSize)) {
		Log::error("Failed to update the vertex buffer");
		core_free(indicesBuf);
		core_free(verticesBuf);
		core_free(normalsBuf);
		return false;
	}
	core_free(verticesBuf);

	if (state._normalBufferIndex[type] != -1) {
		Log::debug("update normalbuffer: %i (type: %i)", idx, type);
		if (!state._vertexBuffer[type].update(state._normalBufferIndex[type], normalsBuf, normalsBufSize)) {
			Log::error("Failed to update the normal buffer");
			core_free(normalsBuf);
			core_free(indicesBuf);
			return false;
		}
	}
	core_free(normalsBuf);

	Log::debug("update indexbuffer: %i (type: %i)", idx, type);
	if (!state._vertexBuffer[type].update(state._indexBufferIndex[type], indicesBuf, indicesBufSize)) {
		Log::error("Failed to update the index buffer");
		core_free(indicesBuf);
		return false;
	}
	core_free(indicesBuf);
	return true;
}

void RawVolumeRenderer::setAmbientColor(const glm::vec3 &color) {
	_voxelShaderFragData.ambientColor = color;
}

void RawVolumeRenderer::setDiffuseColor(const glm::vec3 &color) {
	_voxelShaderFragData.diffuseColor = color;
}

// Convert Euler angles (pitch, yaw, roll) to a directional vector
// Note: roll (angle.z) is unused
// Angles are expected in degrees
void RawVolumeRenderer::setSunAngle(const glm::vec3 &angle) {
	const float pitch = glm::radians(angle.x);
	const float yaw = glm::radians(angle.y);

	// Convert spherical coordinates to Cartesian direction
	const glm::vec3 direction = glm::vec3(
		glm::cos(pitch) * glm::cos(yaw),
		glm::sin(pitch),
		glm::cos(pitch) * glm::sin(yaw)
	);

	// Calculate sun position at a reasonable distance from origin
	const float sunDistance = 1000.0f;
	const glm::vec3 sunPosition = direction * sunDistance;

	// Set the shadow system's sun position
	_shadow.setPosition(sunPosition, glm::vec3(0.0f), glm::up());
}

voxel::RawVolume *RawVolumeRenderer::resetVolume(const voxel::MeshStatePtr &meshState, int idx) {
	return setVolume(meshState, idx, nullptr, nullptr, nullptr, true);
}

bool RawVolumeRenderer::updateBufferForVolume(const voxel::MeshStatePtr &meshState, int idx) {
	core_trace_scoped(RawVolumeRendererUpdateBothBuffers);
	bool success = true;
	if (!updateBufferForVolume(meshState, idx, voxel::MeshType_Opaque)) {
		success = false;
	}
	if (!updateBufferForVolume(meshState, idx, voxel::MeshType_Transparency)) {
		success = false;
	}
	return success;
}

void RawVolumeRenderer::clear(const voxel::MeshStatePtr &meshState) {
	core_trace_scoped(RawVolumeRendererClear);
	meshState->clearPendingExtractions();
	for (int i = 0; i < voxel::MAX_VOLUMES; ++i) {
		// TODO: collect the old volumes and allow to let the caller delete them - they might not all be managed by a
		// node or brush
		voxel::RawVolume *old = resetVolume(meshState, i);
		if (old != nullptr) {
			updateBufferForVolume(meshState, i);
		}
	}
	meshState->resetReferences();
	meshState->clearMeshes();
}

void RawVolumeRenderer::updatePalette(const voxel::MeshStatePtr &meshState, int idx) {
	core_trace_scoped(UpdatePalette);
	const int bufferIndex = meshState->resolveIdx(idx);
	const palette::Palette &palette = meshState->palette(bufferIndex);

	if (palette.hash() != _paletteHash) {
		_paletteHash = palette.hash();
		palette.toVec4f(_voxelShaderVertData.materialcolor);
		palette.emitToVec4f(_voxelShaderVertData.materialcolor, _voxelShaderVertData.glowcolor);
		static_assert(lengthof(_voxelShaderVertData.materialcolor) == palette::PaletteMaxColors);
		static_assert(lengthof(_voxelShaderVertData.glowcolor) == palette::PaletteMaxColors);
	}

	const palette::NormalPalette &normalsPalette = meshState->normalsPalette(bufferIndex);
	if (normalsPalette.hash() != _normalsPaletteHash) {
		_normalsPaletteHash = normalsPalette.hash();
		normalsPalette.toVec4f(_voxelShaderVertData.normals);
		static_assert(lengthof(_voxelShaderVertData.normals) == palette::NormalPaletteMaxNormals);
	}
}

void RawVolumeRenderer::updateCulling(const voxel::MeshStatePtr &meshState, int idx, const video::Camera &camera) {
	if (!_cullNodes->boolVal()) {
		return;
	}
	if (meshState->hidden(idx)) {
		_state[idx]._culled = true;
		return;
	}
	core_trace_scoped(UpdateCulling);
	_state[idx]._culled = false;
	_state[idx]._empty = false;
	// check a potentially referenced mesh here
	const int bufferIndex = meshState->resolveIdx(idx);
	if (!_state[bufferIndex].hasData()) {
#if 0
		if (_state[bufferIndex]._rawVolume) {
			Log::trace("No data, but volume: %i", bufferIndex);
		}
#endif
		_state[idx]._empty = true;
		return;
	}
	const glm::ivec3 &mins = meshState->mins(idx);
	const glm::ivec3 &maxs = meshState->maxs(idx);
	const glm::vec3 size = maxs - mins;
	// if no mins/maxs were given, we can't cull
	if (size.x >= 1.0f && size.y >= 1.0f && size.z >= 1.0f) {
		_state[idx]._culled = !camera.isVisible(mins, maxs);
	}
}

bool RawVolumeRenderer::isVisible(const voxel::MeshStatePtr &meshState, int idx, bool hideEmpty) const {
	if (meshState->hidden(idx)) {
		return false;
	}
	if (_state[idx]._culled) {
		return false;
	}
	if (hideEmpty && _state[idx]._empty) {
		return false;
	}
	return true;
}

static inline glm::vec3 centerPos(int x, int y, int z) {
	// we want the center of the voxel
	return {(float)x + 0.5f, (float)y + 0.5f, (float)z + 0.5f};
}

void RawVolumeRenderer::renderNormals(const voxel::MeshStatePtr &meshState, const RenderContext &renderContext, const video::Camera &camera) {
	if (!renderContext.renderNormals) {
		return;
	}

	core_trace_scoped(RenderNormals);
	for (int idx = 0; idx < voxel::MAX_VOLUMES; ++idx) {
		if (!isVisible(meshState, idx)) {
			continue;
		}
		const palette::NormalPalette &normalPalette = meshState->normalsPalette(idx);
		if (normalPalette.size() == 0u) {
			continue;
		}
		if (_state[idx]._dirtyNormals) {
			_shapeBuilder.clear();
			_shapeBuilder.setColor(core::Color::Red());
			if (const voxel::RawVolume *v = meshState->volume(idx)) {
				if (v->region().voxels() < 128 * 128 * 128) {
					const int n = v->region().stride();
					_shapeBuilder.reserve(2 * n, 2 * n);
					voxelutil::visitSurfaceVolume(
						*v, [this, &normalPalette](int x, int y, int z, const voxel::Voxel &voxel) {
							const glm::vec3 &center = centerPos(x, y, z);
							const glm::vec3 &norm = normalPalette.normal3f(voxel.getNormal());
							_shapeBuilder.line(center, center + norm * 3.0f);
						});
				} else {
					Log::debug("Don't create normals for large volumes");
				}
			}
			_shapeRenderer.createOrUpdate(_state[idx]._normalPreviewBufferIndex, _shapeBuilder);
			_state[idx]._dirtyNormals = false;
		}
		glm::mat4 model(1.0f);
		if (renderContext.applyTransforms()) {
			model = meshState->model(idx);
		}
		_shapeRenderer.render(_state[idx]._normalPreviewBufferIndex, camera, model);
	}
}

void RawVolumeRenderer::renderOpaque(const voxel::MeshStatePtr &meshState, const video::Camera &camera) {
	core_trace_scoped(RenderOpaque);
	const video::PolygonMode mode = camera.polygonMode();
	for (int idx = 0; idx < voxel::MAX_VOLUMES; ++idx) {
		if (!isVisible(meshState, idx)) {
			continue;
		}
		const int bufferIndex = meshState->resolveIdx(idx);
		const uint32_t indices = _state[bufferIndex].indices(voxel::MeshType_Opaque);
		if (indices == 0u) {
			if (meshState->volume(bufferIndex)) {
				Log::trace("No indices but volume for idx %d: %d", idx, bufferIndex);
			}
			continue;
		}

		updatePalette(meshState, bufferIndex);
		_voxelShaderVertData.viewprojection = camera.viewProjectionMatrix();
		_voxelShaderVertData.model = meshState->model(idx);
		_voxelShaderVertData.gray = meshState->grayed(idx);
		core_assert_always(_voxelData.update(_voxelShaderVertData));

		video::ScopedPolygonMode polygonMode(mode);
		video::ScopedFaceCull scopedFaceCull(meshState->cullFace(idx));
		video::ScopedBuffer scopedBuf(_state[bufferIndex]._vertexBuffer[voxel::MeshType_Opaque]);
		core_assert(scopedBuf.success());
		if (_voxelNormShader.isActive()) {
			core_assert_always(_voxelNormShader.setFrag(_voxelData.getFragUniformBuffer()));
			core_assert_always(_voxelNormShader.setVert(_voxelData.getVertUniformBuffer()));
			if (_shadowMap->boolVal()) {
				_voxelNormShader.setShadowmap(video::TextureUnit::One);
			}
		} else {
			core_assert_always(_voxelShader.setFrag(_voxelData.getFragUniformBuffer()));
			core_assert_always(_voxelShader.setVert(_voxelData.getVertUniformBuffer()));
			if (_shadowMap->boolVal()) {
				_voxelShader.setShadowmap(video::TextureUnit::One);
			}
		}
		video::drawElements<voxel::IndexType>(video::Primitive::Triangles, indices);
	}
}

void RawVolumeRenderer::renderTransparency(const voxel::MeshStatePtr &meshState, RenderContext &renderContext, const video::Camera &camera) {
	core_trace_scoped(RenderTransparency);
	const video::PolygonMode mode = camera.polygonMode();
	core::Buffer<int> sorted;
	{
		core_trace_scoped(Sort);
		sorted.reserve(voxel::MAX_VOLUMES);
		for (int idx = 0; idx < voxel::MAX_VOLUMES; ++idx) {
			if (!isVisible(meshState, idx)) {
				continue;
			}
			const int bufferIndex = meshState->resolveIdx(idx);
			const uint32_t indices = _state[bufferIndex].indices(voxel::MeshType_Transparency);
			if (indices == 0u) {
				continue;
			}

			sorted.push_back(idx);
		}

		const glm::vec3 &camPos = camera.worldPosition();
		app::sort_parallel(sorted.begin(), sorted.end(), [&camPos, &meshState](int a, int b) {
			const glm::vec3 &posA = meshState->centerPos(a, true);
			const glm::vec3 &posB = meshState->centerPos(b, true);
			const float d1 = glm::distance2(camPos, posA);
			const float d2 = glm::distance2(camPos, posB);
			return d1 > d2;
		});
	}

	video::ScopedState scopedBlendTrans(video::State::Blend, true);
	for (int idx : sorted) {
		const int bufferIndex = meshState->resolveIdx(idx);
		const uint32_t indices = _state[bufferIndex].indices(voxel::MeshType_Transparency);
		updatePalette(meshState, idx);
		_voxelShaderVertData.viewprojection = camera.viewProjectionMatrix();
		_voxelShaderVertData.model = meshState->model(idx);
		_voxelShaderVertData.gray = meshState->grayed(idx);
		core_assert_always(_voxelData.update(_voxelShaderVertData));

		video::ScopedPolygonMode polygonMode(mode);
		video::ScopedFaceCull scopedFaceCull(meshState->cullFace(idx));
		video::ScopedBuffer scopedBuf(_state[bufferIndex]._vertexBuffer[voxel::MeshType_Transparency]);
		if (_voxelNormShader.isActive()) {
			core_assert_always(_voxelNormShader.setFrag(_voxelData.getFragUniformBuffer()));
			core_assert_always(_voxelNormShader.setVert(_voxelData.getVertUniformBuffer()));
			if (_shadowMap->boolVal()) {
				_voxelNormShader.setShadowmap(video::TextureUnit::One);
			}
		} else {
			core_assert_always(_voxelShader.setFrag(_voxelData.getFragUniformBuffer()));
			core_assert_always(_voxelShader.setVert(_voxelData.getVertUniformBuffer()));
			if (_shadowMap->boolVal()) {
				_voxelShader.setShadowmap(video::TextureUnit::One);
			}
		}
		video::drawElements<voxel::IndexType>(video::Primitive::Triangles, indices);
	}
}

// TODO: PERF: this is a huge bottleneck when rendering large scenes with many transparent objects
void RawVolumeRenderer::sortBeforeRender(const voxel::MeshStatePtr &meshState, const video::Camera &camera) {
	core_trace_scoped(RawVolumeRendererSortBeforeRender);
	const voxel::MeshState::MeshesMap &transparencyMeshes = meshState->meshes(voxel::MeshType_Transparency);
	if (transparencyMeshes.empty()) {
		return;
	}
	core::Buffer<int> indices;
	indices.resize(voxel::MAX_VOLUMES);
	for (const auto &i : transparencyMeshes) {
		indices.fill(-1);
		const glm::vec3 worldPosition = camera.worldPosition();
		const voxel::MeshState::Meshes &meshes = i->second;
		for (int idx = 0; idx < voxel::MAX_VOLUMES; ++idx) {
			if (!isVisible(meshState, idx, true)) {
				continue;
			}
			const int bufferIndex = meshState->resolveIdx(idx);
			// TODO: transform - vertices are in object space - eye in world space
			// inverse of state._model - but take pivot into account
			voxel::Mesh *mesh = meshes[bufferIndex];
			if (!mesh || mesh->isEmpty()) {
				continue;
			}
			if (mesh->sort(worldPosition)) {
				indices[idx] = bufferIndex;
			}
		}
		core_trace_scoped(UpdateBuffers);
		for (size_t idx = 0; idx < indices.size(); ++idx) {
			if (indices[idx] == -1) {
				continue;
			}
			if (!updateBufferForVolume(meshState, indices[idx], voxel::MeshType_Transparency, UpdateBufferFlags::Indices)) {
				Log::error("Failed to update the transparency mesh at index %i", (int)idx);
			}
		}
	}
}

void RawVolumeRenderer::render(const voxel::MeshStatePtr &meshState, RenderContext &renderContext,
							   const video::Camera &camera, bool shadow) {
	core_trace_scoped(RawVolumeRendererRender);

	bool visible = false;
	app::for_parallel(0, voxel::MAX_VOLUMES, [&](int start, int end) {
		for (int idx = start; idx < end; ++idx) {
			updateCulling(meshState, idx, camera);
			if (!isVisible(meshState, idx)) {
				continue;
			}
			visible = true;
		}
	});
	if (!visible) {
		return;
	}

	if (_cullBuffers->boolVal()) {
		core_trace_scoped(CullBuffers);
		for (int idx = 0; idx < voxel::MAX_VOLUMES; ++idx) {
			if (!isVisible(meshState, idx)) {
				continue;
			}
			size_t indCount = 0u;
			size_t vertCount = 0u;
			size_t normalsCount = 0u;
			meshState->count(voxel::MeshType_Opaque, idx, vertCount, normalsCount, indCount);
			if (indCount == 0u || vertCount == 0u) {
				continue;
			}
			if (!updateIndexBufferForVolumeCull(meshState, camera, idx, voxel::MeshType_Opaque, indCount)) {
				Log::error("Failed to update the opaque mesh at index %i", idx);
			}
		}
	}

	sortBeforeRender(meshState, camera);

	video::ScopedState scopedDepth(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::ScopedState scopedCullFace(video::State::CullFace);
	video::ScopedState scopedScissor(video::State::Scissor, false);
	video::ScopedState scopedBlend(video::State::Blend, false);
	video::ScopedState scopedDepthMask(video::State::DepthMask);
	if (_shadowMap->boolVal()) {
		_shadow.update(camera, true);
		if (shadow) {
			video::ScopedShader scoped(_shadowMapShader);
			auto renderFunc = [this, &meshState](int depthBufferIndex, const glm::mat4 &lightViewProjection) {
				alignas(16) shader::ShadowmapData::BlockData var;
				var.lightviewprojection = lightViewProjection;

				for (int idx = 0; idx < voxel::MAX_VOLUMES; ++idx) {
					if (!isVisible(meshState, idx)) {
						continue;
					}
					const int bufferIndex = meshState->resolveIdx(idx);
					for (int i = 0; i < voxel::MeshType_Transparency; ++i) { // TODO: do we want this for the transparent voxels, too?
						const uint32_t indices = _state[bufferIndex].indices((voxel::MeshType)i);
						if (indices > 0u) {
							video::ScopedBuffer scopedBuf(_state[bufferIndex]._vertexBuffer[i]);
							var.model = meshState->model(idx);
							_shadowMapUniformBlock.update(var);
							_shadowMapShader.setBlock(_shadowMapUniformBlock.getBlockUniformBuffer());
							video::ScopedFaceCull scopedFaceCull(meshState->cullFace(idx));
							static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
							video::drawElements<voxel::IndexType>(video::Primitive::Triangles, indices);
						}
					}
				}
				return true;
			};
			_shadow.render(renderFunc, true);
		} else {
			auto renderFunc = [](int i, const glm::mat4 &lightViewProjection) {
				video::clear(video::ClearFlag::Depth);
				return true;
			};
			_shadow.render(renderFunc);
		}
	}

	_voxelShaderFragData.depthsize = _shadow.dimension();
	for (int i = 0; i < shader::VoxelShaderConstants::getMaxDepthBuffers(); ++i) {
		_voxelShaderFragData.cascades[i] = _shadow.cascades()[i];
		_voxelShaderFragData.distances[i] = _shadow.distances()[i];
	}
	_voxelShaderFragData.lightdir = _shadow.sunDirection();
	core_assert_always(_voxelData.update(_voxelShaderFragData));

	const voxel::SurfaceExtractionType meshMode = meshState->meshMode();
	const bool normals = meshMode == voxel::SurfaceExtractionType::MarchingCubes;
	video::Id oldShader = video::getProgram();
	if (normals) {
		_voxelNormShader.activate();
	} else {
		_voxelShader.activate();
	}
	if (_shadowMap->boolVal()) {
		core_assert_always(_shadow.bind(video::TextureUnit::One));
	}

	const video::PolygonMode mode = camera.polygonMode();
	if (mode == video::PolygonMode::Points) {
		video::enable(video::State::PolygonOffsetPoint);
	} else if (mode == video::PolygonMode::WireFrame) {
		video::enable(video::State::PolygonOffsetLine);
	} else if (mode == video::PolygonMode::Solid) {
		video::enable(video::State::PolygonOffsetFill);
	}

	_paletteHash = 0;

	// --- opaque pass
	renderOpaque(meshState, camera);

	// --- transparency pass
	renderTransparency(meshState, renderContext, camera);

	if (mode == video::PolygonMode::Points) {
		video::disable(video::State::PolygonOffsetPoint);
	} else if (mode == video::PolygonMode::WireFrame) {
		video::disable(video::State::PolygonOffsetLine);
	} else if (mode == video::PolygonMode::Solid) {
		video::disable(video::State::PolygonOffsetFill);
	}
	if (_bloom->boolVal()) {
		// If multisampling is enabled, resolve the multisampled framebuffer to regular textures first
		if (renderContext.enableMultisampling) {
			const glm::ivec2 fbDim = renderContext.frameBuffer.dimension();
			// Resolve color attachments
			video::blitFramebuffer(renderContext.frameBuffer.handle(), renderContext.resolveFrameBuffer.handle(),
								 video::ClearFlag::Color, fbDim.x, fbDim.y);

			// Use resolved textures for bloom
			video::FrameBuffer &frameBuffer = renderContext.resolveFrameBuffer;
			const video::TexturePtr &color0 = frameBuffer.texture(video::FrameBufferAttachment::Color0);
			const video::TexturePtr &color1 = frameBuffer.texture(video::FrameBufferAttachment::Color1);
			renderContext.bloomRenderer.render(color0, color1);
		} else {
			// Use regular framebuffer textures directly
			video::FrameBuffer &frameBuffer = renderContext.frameBuffer;
			const video::TexturePtr &color0 = frameBuffer.texture(video::FrameBufferAttachment::Color0);
			const video::TexturePtr &color1 = frameBuffer.texture(video::FrameBufferAttachment::Color1);
			renderContext.bloomRenderer.render(color0, color1);
		}
	}
	if (normals) {
		_voxelNormShader.deactivate();
	} else {
		_voxelShader.deactivate();
	}

	renderNormals(meshState, renderContext, camera);
	video::useProgram(oldShader);
}

void RawVolumeRenderer::setVolume(const voxel::MeshStatePtr &meshState, int idx, scenegraph::SceneGraphNode &node, bool deleteMesh) {
	// ignore the return value because the volume is owned by the node
	(void)setVolume(meshState, idx, node.volume(), &node.palette(), &node.normalPalette(), deleteMesh);
}

voxel::RawVolume *RawVolumeRenderer::setVolume(const voxel::MeshStatePtr &meshState, int idx, voxel::RawVolume *volume, palette::Palette *palette,
											   palette::NormalPalette *normalPalette, bool meshDelete) {
	bool meshDeleted = false;
	if (!meshState->sameNormalPalette(idx, normalPalette)) {
		if (idx >= 0 && idx < voxel::MAX_VOLUMES) {
			_state[idx]._dirtyNormals = true;
		}
	}
	voxel::RawVolume *v = meshState->setVolume(idx, volume, palette, normalPalette, meshDelete, meshDeleted);
	if (meshDeleted) {
		deleteMeshes(idx);
	}
	return v;
}

void RawVolumeRenderer::deleteMeshes(int idx) {
	for (int i = 0; i < voxel::MeshType_Max; ++i) {
		deleteMesh(idx, (voxel::MeshType)i);
	}
	_state[idx]._dirtyNormals = true;
}

void RawVolumeRenderer::deleteMesh(int idx, voxel::MeshType meshType) {
	RenderState &state = _state[idx];
	video::Buffer &vertexBuffer = state._vertexBuffer[meshType];
	Log::debug("clear vertexbuffer: %i", idx);

	vertexBuffer.update(state._vertexBufferIndex[meshType], nullptr, 0);
	core_assert(vertexBuffer.size(state._vertexBufferIndex[meshType]) == 0);
	if (state._normalBufferIndex[meshType] != -1) {
		vertexBuffer.update(state._normalBufferIndex[meshType], nullptr, 0);
		core_assert(vertexBuffer.size(state._normalBufferIndex[meshType]) == 0);
	}
	vertexBuffer.update(state._indexBufferIndex[meshType], nullptr, 0);
	core_assert(vertexBuffer.size(state._indexBufferIndex[meshType]) == 0);

	if (state._normalPreviewBufferIndex != -1) {
		_shapeRenderer.deleteMesh(state._normalPreviewBufferIndex);
		state._normalPreviewBufferIndex = -1;
	}
}

void RawVolumeRenderer::shutdownStateBuffers() {
	for (int idx = 0; idx < voxel::MAX_VOLUMES; ++idx) {
		RenderState &state = _state[idx];
		for (int i = 0; i < voxel::MeshType_Max; ++i) {
			state._vertexBuffer[i].shutdown();
			state._vertexBufferIndex[i] = -1;
			state._normalBufferIndex[i] = -1;
			state._indexBufferIndex[i] = -1;
		}
		state._normalPreviewBufferIndex = -1;
	}
}

bool RawVolumeRenderer::resetStateBuffers(bool normals) {
	shutdownStateBuffers();
	return initStateBuffers(normals);
}

void RawVolumeRenderer::shutdown() {
	_voxelShader.shutdown();
	_voxelNormShader.shutdown();
	_shadowMapShader.shutdown();
	_voxelData.shutdown();
	_shadowMapUniformBlock.shutdown();
	_shadow.shutdown();
	shutdownStateBuffers();
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
}

} // namespace voxelrender
