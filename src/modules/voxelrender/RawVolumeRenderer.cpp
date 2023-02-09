/**
 * @file
 */

#include "RawVolumeRenderer.h"
#include "core/Common.h"
#include "core/Trace.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/epsilon.hpp>
#include "video/FrameBufferConfig.h"
#include "video/ScopedFrameBuffer.h"
#include "video/Texture.h"
#include "video/TextureConfig.h"
#include "voxel/ChunkMesh.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelutil/VolumeMerger.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedPolygonMode.h"
#include "ShaderAttribute.h"
#include "video/Camera.h"
#include "video/Types.h"
#include "video/Renderer.h"
#include "video/ScopedState.h"
#include "video/Shader.h"
#include "core/Color.h"
#include "core/ArrayLength.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "core/GameConfig.h"
#include "core/Log.h"
#include "core/Algorithm.h"
#include "core/StandardLib.h"
#include "VoxelShaderConstants.h"
#include <SDL_timer.h>

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

RawVolumeRenderer::RawVolumeRenderer() :
		_voxelShader(shader::VoxelShader::getInstance()),
		_shadowMapShader(shader::ShadowmapShader::getInstance()) {
}

void RawVolumeRenderer::construct() {
	core::Var::get(cfg::VoxelMeshSize, "64", core::CV_READONLY);
}

bool RawVolumeRenderer::init() {
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_bloom = core::Var::getSafe(cfg::ClientBloom);
	_meshSize = core::Var::getSafe(cfg::VoxelMeshSize);

	_threadPool.init();
	Log::debug("Threadpool size: %i", (int)_threadPool.size());

	if (!_voxelShader.setup()) {
		Log::error("Failed to initialize the voxel shader");
		return false;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return false;
	}
	shader::ShadowmapData::BlockData var;
	_shadowMapUniformBlock.create(var);

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		State& state = _state[idx];
		state._model = glm::mat4(1.0f);
		for (int i = 0; i < MeshType_Max; ++i) {
			state._vertexBufferIndex[i] = state._vertexBuffer[i].create();
			if (state._vertexBufferIndex[i] == -1) {
				Log::error("Could not create the vertex buffer object");
				return false;
			}

			state._indexBufferIndex[i] = state._vertexBuffer[i].create(nullptr, 0, video::BufferType::IndexBuffer);
			if (state._indexBufferIndex[i] == -1) {
				Log::error("Could not create the vertex buffer object for the indices");
				return false;
			}
		}
	}

	const int shaderMaterialColorsArraySize = lengthof(shader::VoxelData::VertData::materialcolor);
	if (shaderMaterialColorsArraySize != voxel::PaletteMaxColors) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, voxel::PaletteMaxColors);
		return false;
	}

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		State& state = _state[idx];
		for (int i = 0; i < MeshType_Max; ++i) {
			const video::Attribute& attributePos = getPositionVertexAttribute(
					state._vertexBufferIndex[i], _voxelShader.getLocationPos(),
					_voxelShader.getComponentsPos());
			state._vertexBuffer[i].addAttribute(attributePos);

			const video::Attribute& attributeInfo = getInfoVertexAttribute(
					state._vertexBufferIndex[i], _voxelShader.getLocationInfo(),
					_voxelShader.getComponentsInfo());
			state._vertexBuffer[i].addAttribute(attributeInfo);
		}
	}

	voxelrender::ShadowParameters shadowParams;
	shadowParams.maxDepthBuffers = shader::VoxelShaderConstants::getMaxDepthBuffers();
	if (!_shadow.init(shadowParams)) {
		Log::error("Failed to initialize the shadow object");
		return false;
	}

	_voxelShaderFragData.diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
	_voxelShaderFragData.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);

	_voxelData.create(_voxelShaderFragData);
	_voxelData.create(_voxelShaderVertData);

	return true;
}

bool RawVolumeRenderer::scheduleExtractions(size_t maxExtraction) {
	const size_t n = _extractRegions.size();
	if (n == 0) {
		return false;
	}
	if (maxExtraction == 0) {
		return true;
	}
	size_t i;
	for (i = 0; i < n; ++i) {
		const int idx = _extractRegions[i].idx;
		const voxel::RawVolume *v = volume(idx);
		if (v == nullptr) {
			continue;
		}
		const voxel::Region& finalRegion = _extractRegions[i].region;
		bool onlyAir = true;
		voxel::RawVolume copy(v, voxel::Region(finalRegion.getLowerCorner() - 2, finalRegion.getUpperCorner() + 2), &onlyAir);
		const glm::ivec3& mins = finalRegion.getLowerCorner();
		if (!onlyAir) {
			_threadPool.enqueue([movedCopy = core::move(copy), mins, idx, finalRegion, this] () {
				++_runningExtractorTasks;
				voxel::ChunkMesh mesh(65536, 65536, true);
				voxel::Region extractRegion = finalRegion;
				extractRegion.shiftUpperCorner(1, 1, 1);
				voxel::extractCubicMesh(&movedCopy, extractRegion, &mesh, mins);
				_pendingQueue.emplace(mins, idx, core::move(mesh));
				Log::debug("Enqueue mesh for idx: %i (%i:%i:%i)", idx, mins.x, mins.y, mins.z);
				--_runningExtractorTasks;
			});
		} else {
			_pendingQueue.emplace(mins, idx, core::move(voxel::ChunkMesh(0, 0)));
		}
		--maxExtraction;
		if (maxExtraction == 0) {
			break;
		}
	}
	_extractRegions.erase(0, i + 1);

	return true;
}

void RawVolumeRenderer::update() {
	scheduleExtractions();
	ExtractionCtx result;
	int cnt = 0;
	while (_pendingQueue.pop(result)) {
		Meshes& meshes = _meshes[MeshType_Opaque][result.mins];
		if (meshes[result.idx] != nullptr) {
			delete meshes[result.idx];
		}
		meshes[result.idx] = new voxel::Mesh(core::move(result.mesh.mesh[MeshType_Opaque]));
		if (!updateBufferForVolume(result.idx, MeshType_Opaque)) {
			Log::error("Failed to update the mesh at index %i", result.idx);
		}

		Meshes& meshesT = _meshes[MeshType_Transparency][result.mins];
		if (meshesT[result.idx] != nullptr) {
			delete meshesT[result.idx];
		}
		meshesT[result.idx] = new voxel::Mesh(core::move(result.mesh.mesh[MeshType_Transparency]));
		if (!updateBufferForVolume(result.idx, MeshType_Transparency)) {
			Log::error("Failed to update the mesh at index %i", result.idx);
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

	size_t vertCount = 0u;
	size_t indCount = 0u;
	for (auto& i : _meshes[type]) {
		const Meshes& meshes = i.second;
		const voxel::Mesh* mesh = meshes[idx];
		if (mesh == nullptr || mesh->getNoOfIndices() <= 0) {
			continue;
		}
		const voxel::VertexArray& vertexVector = mesh->getVertexVector();
		const voxel::IndexArray& indexVector = mesh->getIndexVector();
		vertCount += vertexVector.size();
		indCount += indexVector.size();
	}

	State& state = _state[idx];
	if (indCount == 0u || vertCount == 0u) {
		state._vertexBuffer[type].update(state._vertexBufferIndex[type], nullptr, 0);
		state._vertexBuffer[type].update(state._indexBufferIndex[type], nullptr, 0);
		return true;
	}

	const size_t verticesBufSize = vertCount * sizeof(voxel::VoxelVertex);
	voxel::VoxelVertex* verticesBuf = (voxel::VoxelVertex*)core_malloc(verticesBufSize);
	const size_t indicesBufSize = indCount * sizeof(voxel::IndexType);
	voxel::IndexType* indicesBuf = (voxel::IndexType*)core_malloc(indicesBufSize);

	voxel::VoxelVertex* verticesPos = verticesBuf;
	voxel::IndexType* indicesPos = indicesBuf;

	voxel::IndexType offset = (voxel::IndexType)0;
	for (auto& i : _meshes[type]) {
		const Meshes& meshes = i.second;
		const voxel::Mesh* mesh = meshes[idx];
		if (mesh == nullptr || mesh->getNoOfIndices() <= 0) {
			continue;
		}
		const voxel::VertexArray& vertexVector = mesh->getVertexVector();
		const voxel::IndexArray& indexVector = mesh->getIndexVector();
		core_memcpy(verticesPos, &vertexVector[0], vertexVector.size() * sizeof(voxel::VoxelVertex));
		core_memcpy(indicesPos, &indexVector[0], indexVector.size() * sizeof(voxel::IndexType));

		for (size_t i = 0; i < indexVector.size(); ++i) {
			*indicesPos++ += offset;
		}

		verticesPos += vertexVector.size();
		offset += vertexVector.size();
	}

	if (!state._vertexBuffer[type].update(state._vertexBufferIndex[type], verticesBuf, verticesBufSize)) {
		Log::error("Failed to update the vertex buffer");
		core_free(indicesBuf);
		core_free(verticesBuf);
		return false;
	}
	core_free(verticesBuf);

	if (!state._vertexBuffer[type].update(state._indexBufferIndex[type], indicesBuf, indicesBufSize)) {
		Log::error("Failed to update the index buffer");
		core_free(indicesBuf);
		return false;
	}
	core_free(indicesBuf);
	return true;
}

void RawVolumeRenderer::setAmbientColor(const glm::vec3& color) {
	_voxelShaderFragData.ambientColor = color;
}

void RawVolumeRenderer::setDiffuseColor(const glm::vec3& color) {
	_voxelShaderFragData.diffuseColor = color;
}

bool RawVolumeRenderer::empty(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return true;
	}
	for (int i = 0; i < MeshType_Max; ++i) {
		for (auto& i : _meshes[i]) {
			const Meshes& meshes = i.second;
			if (meshes[idx] != nullptr && meshes[idx]->getNoOfIndices() > 0) {
				return false;
			}
		}
	}
	return true;
}

voxel::Region RawVolumeRenderer::calculateExtractRegion(int x, int y, int z, const glm::ivec3& meshSize) const {
	const glm::ivec3 mins(x * meshSize.x, y * meshSize.y, z * meshSize.z);
	const glm::ivec3 maxs = mins + meshSize - 1;
	return voxel::Region{mins, maxs};
}

bool RawVolumeRenderer::extractRegion(int idx, const voxel::Region& region) {
	core_trace_scoped(RawVolumeRendererExtract);
	voxel::RawVolume* v = volume(idx);
	if (v == nullptr) {
		return false;
	}

	const int s = _meshSize->intVal();
	const glm::ivec3 meshSize(s);
	const glm::ivec3 meshSizeMinusOne(s - 1);
	const voxel::Region& completeRegion = v->region();

	// convert to step coordinates that are needed to extract
	// the given region mesh size ranges
	// the boundaries are special - that's why we take care of this with
	// the offset of 1 - see the cubic surface extractor docs
	const glm::ivec3& l = (region.getLowerCorner() - meshSizeMinusOne) / meshSize;
	const glm::ivec3& u = (region.getUpperCorner() + 1) / meshSize;

	Log::debug("modified region: %s", region.toString().c_str());
	for (int x = l.x; x <= u.x; ++x) {
		for (int y = l.y; y <= u.y; ++y) {
			for (int z = l.z; z <= u.z; ++z) {
				const voxel::Region& finalRegion = calculateExtractRegion(x, y, z, meshSize);
				const glm::ivec3& mins = finalRegion.getLowerCorner();

				if (!voxel::intersects(completeRegion, finalRegion)) {
					for (int i = 0; i < MeshType_Max; ++i) {
						auto iter = _meshes[i].find(mins);
						if (iter != _meshes[i].end()) {
							delete iter->second[idx];
							iter->second[idx] = nullptr;
							State& state = _state[idx];
							state._vertexBuffer[i].update(state._vertexBufferIndex[i], nullptr, 0);
							state._vertexBuffer[i].update(state._indexBufferIndex[i], nullptr, 0);
						}
					}
					continue;
				}

				Log::debug("extract region: %s", finalRegion.toString().c_str());
				_extractRegions.emplace_back(finalRegion, idx);
			}
		}
	}
	return true;
}

void RawVolumeRenderer::waitForPendingExtractions() {
	while (_runningExtractorTasks > 0) {
		SDL_Delay(1);
	}
}

void RawVolumeRenderer::clearPendingExtractions() {
	Log::debug("Clear pending extractions");
	_threadPool.abort();
	while (_runningExtractorTasks > 0) {
		SDL_Delay(1);
	}
	_pendingQueue.clear();
}

bool RawVolumeRenderer::hidden(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return true;
	}
	return _state[idx]._hidden;
}

void RawVolumeRenderer::hide(int idx, bool hide) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return;
	}
	_state[idx]._hidden = hide;
}

bool RawVolumeRenderer::grayed(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return true;
	}
	return _state[idx]._gray;
}

void RawVolumeRenderer::gray(int idx, bool gray) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return;
	}
	_state[idx]._gray = gray;
}

void RawVolumeRenderer::updatePalette(int idx) {
	const voxel::Palette *palette;
	const State &state = _state[idx];
	if (state._palette.hasValue()) {
		palette = state._palette.value();
	} else {
		palette = &voxel::getPalette();
	}

	if (palette->hash() != _paletteHash) {
		_paletteHash = palette->hash();
		core::DynamicArray<glm::vec4> materialColors;
		palette->toVec4f(materialColors);
		core::DynamicArray<glm::vec4> glowColors;
		palette->glowToVec4f(glowColors);
		for (int i = 0; i < voxel::PaletteMaxColors; ++i) {
			_voxelShaderVertData.materialcolor[i] = materialColors[i];
			_voxelShaderVertData.glowcolor[i] = glowColors[i];
		}
	}
}

void RawVolumeRenderer::render(RenderContext &renderContext, const video::Camera& camera, bool shadow) {
	core_trace_scoped(RawVolumeRendererRender);

	bool visible = false;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		const State& state = _state[idx];
		if (state._hidden) {
			continue;
		}
		if (!state.hasData()) {
			continue;
		}
		visible = true;
	}
	if (!visible) {
		if (_bloom->boolVal()) {
			video::ScopedFrameBuffer scoped(renderContext.frameBuffer);
			video::clear(video::ClearFlag::Color);
		}
		return;
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
			_shadow.render([this] (int i, const glm::mat4& lightViewProjection) {
				shader::ShadowmapData::BlockData var;
				var.lightviewprojection = lightViewProjection;

				for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
					const State& state = _state[idx];
					const uint32_t indices = state.indices(MeshType_Opaque);
					if (indices == 0u) {
						continue;
					}
					video::ScopedBuffer scopedBuf(state._vertexBuffer[MeshType_Opaque]);
					var.model = state._model;
					var.pivot = state._pivot;
					_shadowMapUniformBlock.update(var);
					_shadowMapShader.setBlock(_shadowMapUniformBlock.getBlockUniformBuffer());
					static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
					video::drawElements<voxel::IndexType>(video::Primitive::Triangles, indices);
				}
				return true;
			}, true);
		} else {
			_shadow.render([] (int i, const glm::mat4& lightViewProjection) {
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
	_voxelData.update(_voxelShaderFragData);

	video::ScopedShader scoped(_voxelShader);
	if (_shadowMap->boolVal()) {
		_shadow.bind(video::TextureUnit::One);
	}

	if (_bloom->boolVal()) {
		renderContext.frameBuffer.bind(true);
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
		const State& state = _state[idx];
		const uint32_t indices = state.indices(MeshType_Opaque);
		if (indices == 0u) {
			continue;
		}
		video::ScopedPolygonMode polygonMode(mode);
		video::ScopedBuffer scopedBuf(state._vertexBuffer[MeshType_Opaque]);
		updatePalette(idx);
		_voxelShaderVertData.viewprojection = camera.viewProjectionMatrix();
		_voxelShaderVertData.model = state._model;
		_voxelShaderVertData.pivot = state._pivot;
		_voxelShaderVertData.gray = state._gray;
		core_assert_always(_voxelData.update(_voxelShaderVertData));
		core_assert_always(_voxelShader.setFrag(_voxelData.getFragUniformBuffer()));
		core_assert_always(_voxelShader.setVert(_voxelData.getVertUniformBuffer()));
		if (_shadowMap->boolVal()) {
			_voxelShader.setShadowmap(video::TextureUnit::One);
		}
		video::drawElements<voxel::IndexType>(video::Primitive::Triangles, indices);
	}

	// --- transparency pass
	{
		video::ScopedState scopedBlend(video::State::Blend, true);
		for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
			const State& state = _state[idx];
			const uint32_t indices = state.indices(MeshType_Transparency);
			if (indices == 0u) {
				continue;
			}
			// TODO: alpha support - sort according to eye pos
			video::ScopedPolygonMode polygonMode(mode);
			video::ScopedBuffer scopedBuf(state._vertexBuffer[MeshType_Transparency]);
			updatePalette(idx);
			_voxelShaderVertData.viewprojection = camera.viewProjectionMatrix();
			_voxelShaderVertData.model = state._model;
			_voxelShaderVertData.pivot = state._pivot;
			_voxelShaderVertData.gray = state._gray;
			core_assert_always(_voxelData.update(_voxelShaderVertData));
			core_assert_always(_voxelShader.setFrag(_voxelData.getFragUniformBuffer()));
			core_assert_always(_voxelShader.setVert(_voxelData.getVertUniformBuffer()));
			if (_shadowMap->boolVal()) {
				_voxelShader.setShadowmap(video::TextureUnit::One);
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
		renderContext.frameBuffer.unbind();
		const video::TexturePtr& color0 = renderContext.frameBuffer.texture(video::FrameBufferAttachment::Color0);
		const video::TexturePtr& color1 = renderContext.frameBuffer.texture(video::FrameBufferAttachment::Color1);
		renderContext.bloomRenderer.render(color0, color1);
		video::blitFramebuffer(renderContext.frameBuffer.handle(), video::currentFramebuffer(), video::ClearFlag::Depth, renderContext.frameBuffer.dimension().x, renderContext.frameBuffer.dimension().y);
	}
}

bool RawVolumeRenderer::setModelMatrix(int idx, const glm::mat4& model, const glm::vec3 &pivot, bool reset) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		Log::error("Given id %i is out of bounds", idx);
		return false;
	}
	State& state = _state[idx];
	if (state._rawVolume == nullptr) {
		Log::error("No volume found at: %i", idx);
		return false;
	}
	state._model = model;
	state._pivot = pivot;
	return true;
}

voxel::Region RawVolumeRenderer::region() const {
	voxel::Region region;
	bool validVolume = false;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		const voxel::RawVolume* v = volume(idx);
		if (v == nullptr) {
			continue;
		}
		if (validVolume) {
			region.accumulate(v->region());
			continue;
		}
		region = v->region();
		validVolume = true;
	}
	return region;
}

voxel::RawVolume* RawVolumeRenderer::setVolume(int idx, voxelformat::SceneGraphNode& node, bool deleteMesh) {
	return setVolume(idx, node.volume(), &node.palette(), deleteMesh);
}

voxel::RawVolume* RawVolumeRenderer::setVolume(int idx, voxel::RawVolume* volume, voxel::Palette* palette, bool deleteMesh) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	State& state = _state[idx];
	state._palette.setValue(palette);

	voxel::RawVolume* old = state._rawVolume;
	if (old == volume) {
		return nullptr;
	}
	core_trace_scoped(RawVolumeRendererSetVolume);
	state._rawVolume = volume;
	if (deleteMesh) {
		for (int i = 0; i < MeshType_Max; ++i) {
			for (auto& iter : _meshes[i]) {
				delete iter.second[idx];
				iter.second[idx] = nullptr;
				State& state = _state[idx];
				state._vertexBuffer[i].update(state._vertexBufferIndex[i], nullptr, 0);
				state._vertexBuffer[i].update(state._indexBufferIndex[i], nullptr, 0);
			}
		}
	}
	const size_t n = _extractRegions.size();
	for (size_t i = 0; i < n; ++i) {
		if (_extractRegions[i].idx == idx) {
			_extractRegions[i].idx = -1;
		}
	}

	return old;
}

void RawVolumeRenderer::setSunPosition(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up) {
	_shadow.setPosition(eye, center, up);
}

core::DynamicArray<voxel::RawVolume*> RawVolumeRenderer::shutdown() {
	_threadPool.shutdown();
	_voxelData.shutdown();
	_voxelShader.shutdown();
	_shadowMapShader.shutdown();
	_voxelData.shutdown();
	for (int i = 0; i < MeshType_Max; ++i) {
		for (auto& iter : _meshes[i]) {
			for (auto& mesh : iter.second) {
				delete mesh;
			}
		}
		_meshes[i].clear();
	}
	core::DynamicArray<voxel::RawVolume*> old(MAX_VOLUMES);
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		State& state = _state[idx];
		for (int i = 0; i < MeshType_Max; ++i) {
			state._vertexBuffer[i].shutdown();
			state._vertexBufferIndex[i] = -1;
			state._indexBufferIndex[i] = -1;
		}
		// hand over the ownership to the caller
		old.push_back(state._rawVolume);
		state._rawVolume = nullptr;
	}
	_shadow.shutdown();
	return old;
}

}
