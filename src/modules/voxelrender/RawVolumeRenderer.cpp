/**
 * @file
 */

#include "RawVolumeRenderer.h"
#include "ShaderAttribute.h"
#include "VoxelShaderConstants.h"
#include "core/Algorithm.h"
#include "core/ArrayLength.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/Trace.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Camera.h"
#include "video/FrameBufferConfig.h"
#include "video/Renderer.h"
#include "video/ScopedFrameBuffer.h"
#include "video/ScopedPolygonMode.h"
#include "video/ScopedState.h"
#include "video/Shader.h"
#include "video/Texture.h"
#include "video/TextureConfig.h"
#include "video/Types.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/SurfaceExtractor.h"
#include <SDL_timer.h>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/epsilon.hpp>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/norm.hpp>

namespace voxelrender {

bool RenderContext::init(const glm::ivec2 &size) {
	video::FrameBufferConfig cfg;
	cfg.dimension(size);
	// TODO: add multisampling support
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
	cfg.depthBuffer(true);
	if (!frameBuffer.init(cfg)) {
		Log::error("Failed to initialize the volume renderer bloom framebuffer");
		return false;
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
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
	cfg.depthBuffer(true);
	if (!frameBuffer.init(cfg)) {
		Log::error("Failed to initialize the volume renderer bloom framebuffer");
		return false;
	}

	// we have to do an y-flip here due to the framebuffer handling
	if (!bloomRenderer.resize(size.x, size.y)) {
		Log::error("Failed to initialize the bloom renderer");
		return false;
	}
	return true;
}

void RenderContext::shutdown() {
	frameBuffer.shutdown();
	bloomRenderer.shutdown();
}

RawVolumeRenderer::RawVolumeRenderer(const MeshStatePtr &meshState)
	: _voxelShader(shader::VoxelShader::getInstance()), _voxelNormShader(shader::VoxelnormShader::getInstance()),
	  _shadowMapShader(shader::ShadowmapShader::getInstance()) {
	_meshState = meshState;
}

RawVolumeRenderer::RawVolumeRenderer() : RawVolumeRenderer(core::make_shared<MeshState>()) {
}

void RawVolumeRenderer::construct() {
	_meshState->construct();
}

bool RawVolumeRenderer::initStateBuffers() {
	const voxel::SurfaceExtractionType meshMode = _meshState->meshMode();
	const bool normals = meshMode != voxel::SurfaceExtractionType::Cubic;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		State &state = _state[idx];
		_meshState->setModel(idx, glm::mat4(1.0f));
		for (int i = 0; i < MeshType_Max; ++i) {
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
		}
	}

	if (normals) {
		for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
			State &state = _state[idx];
			for (int i = 0; i < MeshType_Max; ++i) {
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
		for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
			State &state = _state[idx];
			for (int i = 0; i < MeshType_Max; ++i) {
				const video::Attribute &attributePos = getPositionVertexAttribute(
					state._vertexBufferIndex[i], _voxelShader.getLocationPos(), _voxelShader.getComponentsPos());
				state._vertexBuffer[i].addAttribute(attributePos);

				const video::Attribute &attributeInfo = getInfoVertexAttribute(
					state._vertexBufferIndex[i], _voxelShader.getLocationInfo(), _voxelShader.getComponentsInfo());
				state._vertexBuffer[i].addAttribute(attributeInfo);
			}
		}
	}

	return true;
}

bool RawVolumeRenderer::init() {
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_bloom = core::Var::getSafe(cfg::ClientBloom);

	_meshState->init();

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

	if (!initStateBuffers()) {
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

	return true;
}

void RawVolumeRenderer::extractRegion(int idx, const voxel::Region &region) {
	if (_meshState->extractRegion(idx, region)) {
		deleteMeshes(idx);
	}
}

void RawVolumeRenderer::update() {
	if (_meshState->update()) {
		resetStateBuffers();
	}

	int cnt = 0;
	for (;;) {
		const int idx = _meshState->pop();
		if (idx == -1) {
			break;
		}
		if (!updateBufferForVolume(idx, MeshType_Opaque)) {
			Log::error("Failed to update the mesh at index %i", idx);
		}
		if (!updateBufferForVolume(idx, MeshType_Transparency)) {
			Log::error("Failed to update the mesh at index %i", idx);
		}
		++cnt;
	}
	if (cnt > 0) {
		Log::debug("Perform %i mesh updates in this frame", cnt);
	}
}

bool RawVolumeRenderer::updateBufferForVolume(int idx, MeshType type) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	core_trace_scoped(RawVolumeRendererUpdate);

	const int bufferIndex = _meshState->resolveIdx(idx);
	size_t vertCount = 0u;
	size_t normalsCount = 0u;
	size_t indCount = 0u;
	_meshState->count(type, bufferIndex, vertCount, normalsCount, indCount);

	State &state = _state[bufferIndex];
	if (indCount == 0u || vertCount == 0u) {
		Log::debug("clear vertexbuffer: %i", idx);
		video::Buffer &buffer = state._vertexBuffer[type];
		buffer.update(state._vertexBufferIndex[type], nullptr, 0);
		buffer.update(state._normalBufferIndex[type], nullptr, 0);
		buffer.update(state._indexBufferIndex[type], nullptr, 0);
		return true;
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
	for (const auto &i : _meshState->meshes(type)) {
		const MeshState::Meshes &meshes = i->second;
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

	Log::debug("update vertexbuffer: %i (type: %i)", idx, type);
	if (!state._vertexBuffer[type].update(state._vertexBufferIndex[type], verticesBuf, verticesBufSize)) {
		Log::error("Failed to update the vertex buffer");
		core_free(indicesBuf);
		core_free(verticesBuf);
		return false;
	}
	core_free(verticesBuf);

	if (state._normalBufferIndex[type] != -1) {
		Log::debug("update normalbuffer: %i (type: %i)", idx, type);
		if (!state._vertexBuffer[type].update(state._normalBufferIndex[type], normalsBuf, normalsBufSize)) {
			Log::error("Failed to update the normal buffer");
			core_free(normalsBuf);
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

void RawVolumeRenderer::resetVolume(int idx) {
	setVolume(idx, nullptr, nullptr, true);
}

bool RawVolumeRenderer::updateBufferForVolume(int idx) {
	bool success = true;
	if (!updateBufferForVolume(idx, MeshType_Opaque)) {
		success = false;
	}
	if (!updateBufferForVolume(idx, MeshType_Transparency)) {
		success = false;
	}
	return success;
}

void RawVolumeRenderer::clear() {
	_meshState->clearPendingExtractions();
	for (int i = 0; i < MAX_VOLUMES; ++i) {
		// TODO: collect the old volumes and allow to let the caller delete them - they might not all be managed by a
		// node
		voxel::RawVolume *old = setVolume(i, nullptr, nullptr, true);
		if (old != nullptr) {
			updateBufferForVolume(i);
		}
	}
	_meshState->resetReferences();
}

void RawVolumeRenderer::updatePalette(int idx) {
	const int bufferIndex = _meshState->resolveIdx(idx);
	const palette::Palette &palette = _meshState->palette(bufferIndex);

	if (palette.hash() != _paletteHash) {
		_paletteHash = palette.hash();
		core::DynamicArray<glm::vec4> materialColors;
		palette.toVec4f(materialColors);
		core::DynamicArray<glm::vec4> glowColors;
		palette.emitToVec4f(glowColors);
		for (int i = 0; i < palette::PaletteMaxColors; ++i) {
			_voxelShaderVertData.materialcolor[i] = materialColors[i];
			_voxelShaderVertData.glowcolor[i] = glowColors[i];
		}
	}
}

void RawVolumeRenderer::updateCulling(int idx, const video::Camera &camera) {
	if (_meshState->hidden(idx)) {
		_state[idx]._culled = true;
		return;
	}
	_state[idx]._culled = false;
	// check a potentially referenced mesh here
	const int bufferIndex = _meshState->resolveIdx(idx);
	if (!_state[bufferIndex].hasData()) {
#if 0
		if (_state[bufferIndex]._rawVolume) {
			Log::trace("No data, but volume: %i", bufferIndex);
		}
#endif
		_state[idx]._culled = true;
		return;
	}
	const glm::ivec3 &mins = _meshState->mins(idx);
	const glm::ivec3 &maxs = _meshState->maxs(idx);
	const glm::vec3 size = maxs - mins;
	// if no mins/maxs were given, we can't cull
	if (size.x >= 1.0f && size.y >= 1.0f && size.z >= 1.0f) {
		_state[idx]._culled = !camera.isVisible(mins, maxs);
	}
}

bool RawVolumeRenderer::isVisible(int idx) const {
	if (_meshState->hidden(idx)) {
		return false;
	}
	if (_state[idx]._culled) {
		return false;
	}
	return true;
}

void RawVolumeRenderer::render(RenderContext &renderContext, const video::Camera &camera, bool shadow) {
	core_trace_scoped(RawVolumeRendererRender);

	bool visible = false;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		updateCulling(idx, camera);
		if (!isVisible(idx)) {
			continue;
		}
		visible = true;
	}
	if (!visible) {
		return;
	}
	for (const auto &i : _meshState->meshes(MeshType_Transparency)) {
		for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
			if (!isVisible(idx)) {
				continue;
			}
			const int bufferIndex = _meshState->resolveIdx(idx);
			// TODO: transform - vertices are in object space - eye in world space
			// inverse of state._model - but take pivot into account
			voxel::Mesh *mesh = i->second[bufferIndex];
			if (!mesh || mesh->isEmpty()) {
				continue;
			}
			if (mesh->sort(camera.worldPosition())) {
				updateBufferForVolume(bufferIndex, MeshType_Transparency);
			}
		}
	}

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
			_shadow.render(
				[this](int depthBufferIndex, const glm::mat4 &lightViewProjection) {
					alignas(16) shader::ShadowmapData::BlockData var;
					var.lightviewprojection = lightViewProjection;

					for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
						if (!isVisible(idx)) {
							continue;
						}
						const int bufferIndex = _meshState->resolveIdx(idx);
						for (int i = 0; i < MeshType_Transparency; ++i) { // TODO: do we want this for the transparent voxels, too?
							const uint32_t indices = _state[bufferIndex].indices((MeshType)i);
							if (indices > 0u) {
								video::ScopedBuffer scopedBuf(_state[bufferIndex]._vertexBuffer[i]);
								var.model = _meshState->model(idx);
								var.pivot = _meshState->pivot(idx);
								_shadowMapUniformBlock.update(var);
								_shadowMapShader.setBlock(_shadowMapUniformBlock.getBlockUniformBuffer());
								static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
								video::drawElements<voxel::IndexType>(video::Primitive::Triangles, indices);
							}
						}
					}
					return true;
				},
				true);
		} else {
			_shadow.render([](int i, const glm::mat4 &lightViewProjection) {
				video::clear(video::ClearFlag::Depth);
				return true;
			});
		}
	}

	_voxelShaderFragData.depthsize = _shadow.dimension();
	for (int i = 0; i < shader::VoxelShaderConstants::getMaxDepthBuffers(); ++i) {
		_voxelShaderFragData.cascades[i] = _shadow.cascades()[i];
		_voxelShaderFragData.distances[i] = _shadow.distances()[i];
	}
	_voxelShaderFragData.lightdir = _shadow.sunDirection();
	core_assert_always(_voxelData.update(_voxelShaderFragData));

	const voxel::SurfaceExtractionType meshMode = _meshState->meshMode();
	const bool normals = meshMode != voxel::SurfaceExtractionType::Cubic;
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
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		if (!isVisible(idx)) {
			continue;
		}
		const int bufferIndex = _meshState->resolveIdx(idx);
		const uint32_t indices = _state[bufferIndex].indices(MeshType_Opaque);
		if (indices == 0u) {
			if (_meshState->volume(bufferIndex)) {
				Log::debug("No indices but volume for idx %d: %d", idx, bufferIndex);
			}
			continue;
		}

		updatePalette(bufferIndex);
		_voxelShaderVertData.viewprojection = camera.viewProjectionMatrix();
		_voxelShaderVertData.model = _meshState->model(idx);
		_voxelShaderVertData.pivot = _meshState->pivot(idx);
		_voxelShaderVertData.gray = _meshState->grayed(idx);
		core_assert_always(_voxelData.update(_voxelShaderVertData));

		video::ScopedPolygonMode polygonMode(mode);
		video::ScopedBuffer scopedBuf(_state[bufferIndex]._vertexBuffer[MeshType_Opaque]);
		if (normals) {
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

	// --- transparency pass
	{
		core::DynamicArray<int> sorted;
		sorted.reserve(MAX_VOLUMES);
		for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
			if (!isVisible(idx)) {
				continue;
			}
			const int bufferIndex = _meshState->resolveIdx(idx);
			const uint32_t indices = _state[bufferIndex].indices(MeshType_Transparency);
			if (indices == 0u) {
				continue;
			}

			sorted.push_back(idx);
		}

		const glm::vec3 &camPos = camera.worldPosition();
		core::sort(sorted.begin(), sorted.end(), [this, &camPos](int a, int b) {
			const glm::vec3 &posA = _meshState->centerPos(a);
			const glm::vec3 &posB = _meshState->centerPos(b);
			const float d1 = glm::distance2(camPos, posA);
			const float d2 = glm::distance2(camPos, posB);
			return d1 > d2;
		});

		video::ScopedState scopedBlendTrans(video::State::Blend, true);
		for (int idx : sorted) {
			const int bufferIndex = _meshState->resolveIdx(idx);
			const uint32_t indices = _state[bufferIndex].indices(MeshType_Transparency);
			updatePalette(idx);
			_voxelShaderVertData.viewprojection = camera.viewProjectionMatrix();
			_voxelShaderVertData.model = _meshState->model(idx);
			_voxelShaderVertData.pivot = _meshState->pivot(idx);
			_voxelShaderVertData.gray = _meshState->grayed(idx);
			core_assert_always(_voxelData.update(_voxelShaderVertData));

			// TODO: alpha support - sort according to eye pos
			video::ScopedPolygonMode polygonMode(mode);
			video::ScopedBuffer scopedBuf(_state[bufferIndex]._vertexBuffer[MeshType_Transparency]);
			if (normals) {
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

	if (mode == video::PolygonMode::Points) {
		video::disable(video::State::PolygonOffsetPoint);
	} else if (mode == video::PolygonMode::WireFrame) {
		video::disable(video::State::PolygonOffsetLine);
	} else if (mode == video::PolygonMode::Solid) {
		video::disable(video::State::PolygonOffsetFill);
	}
	if (_bloom->boolVal()) {
		video::FrameBuffer &frameBuffer = renderContext.frameBuffer;
		const video::TexturePtr &color0 = frameBuffer.texture(video::FrameBufferAttachment::Color0);
		const video::TexturePtr &color1 = frameBuffer.texture(video::FrameBufferAttachment::Color1);
		renderContext.bloomRenderer.render(color0, color1);
	}
	if (normals) {
		_voxelNormShader.deactivate();
	} else {
		_voxelShader.deactivate();
	}
	video::useProgram(oldShader);
}

void RawVolumeRenderer::setVolume(int idx, const scenegraph::SceneGraphNode &node, bool deleteMesh) {
	setVolume(idx, node.volume(), &node.palette(), deleteMesh);
}

voxel::RawVolume *RawVolumeRenderer::setVolume(int idx, voxel::RawVolume *volume, palette::Palette *palette,
											   bool meshDelete) {
	bool meshDeleted = false;
	voxel::RawVolume *v = _meshState->setVolume(idx, volume, palette, meshDelete, meshDeleted);
	if (meshDeleted) {
		deleteMeshes(idx);
	}
	return v;
}

void RawVolumeRenderer::deleteMeshes(int idx) {
	for (int i = 0; i < MeshType_Max; ++i) {
		deleteMesh(idx, (MeshType)i);
	}
}

void RawVolumeRenderer::deleteMesh(int idx, MeshType meshType) {
	State &state = _state[idx];
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
}

void RawVolumeRenderer::setSunPosition(const glm::vec3 &eye, const glm::vec3 &center, const glm::vec3 &up) {
	_shadow.setPosition(eye, center, up);
}

void RawVolumeRenderer::shutdownStateBuffers() {
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		State &state = _state[idx];
		for (int i = 0; i < MeshType_Max; ++i) {
			state._vertexBuffer[i].shutdown();
			state._vertexBufferIndex[i] = -1;
			state._normalBufferIndex[i] = -1;
			state._indexBufferIndex[i] = -1;
		}
	}
}

bool RawVolumeRenderer::resetStateBuffers() {
	shutdownStateBuffers();
	return initStateBuffers();
}

core::DynamicArray<voxel::RawVolume *> RawVolumeRenderer::shutdown() {
	_voxelShader.shutdown();
	_voxelNormShader.shutdown();
	_shadowMapShader.shutdown();
	_voxelData.shutdown();
	_shadowMapUniformBlock.shutdown();
	_shadow.shutdown();
	const core::DynamicArray<voxel::RawVolume *> &old = _meshState->shutdown();
	shutdownStateBuffers();
	return old;
}

} // namespace voxelrender
