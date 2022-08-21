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
#include "voxel/IsQuadNeeded.h"
#include <SDL.h>

namespace voxelrender {

RawVolumeRenderer::RawVolumeRenderer() :
		_voxelShader(shader::VoxelInstancedShader::getInstance()),
		_shadowMapShader(shader::ShadowmapInstancedShader::getInstance()) {
}

void RawVolumeRenderer::construct() {
	core::Var::get(cfg::VoxelMeshSize, "64", core::CV_READONLY);
}

bool RawVolumeRenderer::resize(const glm::ivec2 &size) {
	if (_frameBuffer.dimension() == size) {
		return true;
	}
	_frameBuffer.shutdown();
	video::FrameBufferConfig cfg;
	cfg.dimension(size);
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
	cfg.depthBuffer(true);
	if (!_frameBuffer.init(cfg)) {
		Log::error("Failed to initialize the volume renderer bloom framebuffer");
		return false;
	}

	// we have to do an y-flip here due to the framebuffer handling
	if (!_bloomRenderer.resize(size.x, size.y)) {
		Log::error("Failed to initialize the bloom renderer");
		return false;
	}
	return true;
}

bool RawVolumeRenderer::init(const glm::ivec2 &size) {
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_bloom = core::Var::getSafe(cfg::ClientBloom);
	_meshSize = core::Var::getSafe(cfg::VoxelMeshSize);

	_threadPool.init();
	Log::debug("Threadpool size: %i", (int)_threadPool.size());

	if (!_voxelShader.setup()) {
		Log::error("Failed to initialize the world shader");
		return false;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return false;
	}

	video::FrameBufferConfig cfg;
	cfg.dimension(size);
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
	cfg.depthBuffer(true);
	if (!_frameBuffer.init(cfg)) {
		Log::error("Failed to initialize the volume renderer bloom framebuffer");
		return false;
	}

	// we have to do an y-flip here due to the framebuffer handling
	if (!_bloomRenderer.init(true, size.x, size.y)) {
		Log::error("Failed to initialize the bloom renderer");
		return false;
	}

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		State& state = _state[idx];
		state._models[0] = glm::mat4(1.0f);
		state._amounts = 1;
		state._vertexBufferIndex = state._vertexBuffer.create();
		if (state._vertexBufferIndex == -1) {
			Log::error("Could not create the vertex buffer object");
			return false;
		}

		state._indexBufferIndex = state._vertexBuffer.create(nullptr, 0, video::BufferType::IndexBuffer);
		if (state._indexBufferIndex == -1) {
			Log::error("Could not create the vertex buffer object for the indices");
			return false;
		}
	}

	const int shaderMaterialColorsArraySize = lengthof(shader::VoxelData::MaterialblockData::materialcolor);
	if (shaderMaterialColorsArraySize != voxel::PaletteMaxColors) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, voxel::PaletteMaxColors);
		return false;
	}

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		State& state = _state[idx];
		const video::Attribute& attributePos = getPositionVertexAttribute(
				state._vertexBufferIndex, _voxelShader.getLocationPos(),
				_voxelShader.getComponentsPos());
		state._vertexBuffer.addAttribute(attributePos);

		const video::Attribute& attributeInfo = getInfoVertexAttribute(
				state._vertexBufferIndex, _voxelShader.getLocationInfo(),
				_voxelShader.getComponentsInfo());
		state._vertexBuffer.addAttribute(attributeInfo);
	}

	render::ShadowParameters shadowParams;
	shadowParams.maxDepthBuffers = shader::VoxelShaderConstants::getMaxDepthBuffers();
	if (!_shadow.init(shadowParams)) {
		Log::error("Failed to initialize the shadow object");
		return false;
	}

	shader::VoxelData::MaterialblockData materialBlock;
	core_memset(materialBlock.materialcolor, 0, sizeof(materialBlock.materialcolor));
	core_memset(materialBlock.glowcolor, 0, sizeof(materialBlock.glowcolor));
	_materialBlock.create(materialBlock);

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
				voxel::Mesh mesh(65536, 65536, true);
				voxel::extractCubicMesh(&movedCopy, finalRegion, &mesh, voxel::IsQuadNeeded(), mins);
				_pendingQueue.emplace(mins, idx, core::move(mesh));
				Log::debug("Enqueue mesh for idx: %i (%i:%i:%i)", idx, mins.x, mins.y, mins.z);
				--_runningExtractorTasks;
			});
		} else {
			_pendingQueue.emplace(mins, idx, core::move(voxel::Mesh()));
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
		Meshes& meshes = _meshes[result.mins];
		if (meshes[result.idx] != nullptr) {
			delete meshes[result.idx];
		}
		meshes[result.idx] = new voxel::Mesh(core::move(result.mesh));
		if (!updateBufferForVolume(result.idx)) {
			Log::error("Failed to update the mesh at index %i", result.idx);
		}
		++cnt;
	}
	if (cnt > 0) {
		Log::debug("Perform %i mesh updates in this frame", cnt);
	}
}

bool RawVolumeRenderer::updateBufferForVolume(int idx) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	core_trace_scoped(RawVolumeRendererUpdate);

	size_t vertCount = 0u;
	size_t indCount = 0u;
	for (auto& i : _meshes) {
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
		state._vertexBuffer.update(state._vertexBufferIndex, nullptr, 0);
		state._vertexBuffer.update(state._indexBufferIndex, nullptr, 0);
		return true;
	}

	const size_t verticesBufSize = vertCount * sizeof(voxel::VoxelVertex);
	voxel::VoxelVertex* verticesBuf = (voxel::VoxelVertex*)core_malloc(verticesBufSize);
	const size_t indicesBufSize = indCount * sizeof(voxel::IndexType);
	voxel::IndexType* indicesBuf = (voxel::IndexType*)core_malloc(indicesBufSize);

	voxel::VoxelVertex* verticesPos = verticesBuf;
	voxel::IndexType* indicesPos = indicesBuf;

	voxel::IndexType offset = (voxel::IndexType)0;
	for (auto& i : _meshes) {
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

	if (!state._vertexBuffer.update(state._vertexBufferIndex, verticesBuf, verticesBufSize)) {
		Log::error("Failed to update the vertex buffer");
		core_free(indicesBuf);
		core_free(verticesBuf);
		return false;
	}
	core_free(verticesBuf);

	if (!state._vertexBuffer.update(state._indexBufferIndex, indicesBuf, indicesBufSize)) {
		Log::error("Failed to update the index buffer");
		core_free(indicesBuf);
		return false;
	}
	core_free(indicesBuf);
	return true;
}

bool RawVolumeRenderer::updateBufferForVolume(int idx, const voxel::VertexArray& vertices, const voxel::IndexArray& indices) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	core_trace_scoped(RawVolumeRendererUpdate);

	State& state = _state[idx];
	if (indices.empty() || vertices.empty()) {
		state._vertexBuffer.update(state._vertexBufferIndex, nullptr, 0);
		state._vertexBuffer.update(state._indexBufferIndex, nullptr, 0);
		return true;
	}

	if (!state._vertexBuffer.update(state._vertexBufferIndex, &vertices.front(), vertices.size() * sizeof(voxel::VertexArray::value_type))) {
		Log::error("Failed to update the vertex buffer");
		return false;
	}
	if (!state._vertexBuffer.update(state._indexBufferIndex, &indices.front(), indices.size() * sizeof(voxel::IndexArray::value_type))) {
		Log::error("Failed to update the index buffer");
		return false;
	}
	return true;
}

void RawVolumeRenderer::setAmbientColor(const glm::vec3& color) {
	if (glm::all(glm::epsilonEqual(_ambientColor, color, 0.001f))) {
		return;
	}
	_ambientColor = color;
	// force updating the cached uniform values
	_voxelShader.markDirty();
}

void RawVolumeRenderer::setDiffuseColor(const glm::vec3& color) {
	if (glm::all(glm::epsilonEqual(_diffuseColor, color, 0.001f))) {
		return;
	}
	_diffuseColor = color;
	// force updating the cached uniform values
	_voxelShader.markDirty();
}

bool RawVolumeRenderer::empty(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return true;
	}
	for (auto& i : _meshes) {
		const Meshes& meshes = i.second;
		if (meshes[idx] != nullptr && meshes[idx]->getNoOfIndices() > 0) {
			return false;
		}
	}
	return true;
}

bool RawVolumeRenderer::toMesh(int idx, voxel::Mesh* mesh) {
	voxel::RawVolume* v = volume(idx);
	if (v == nullptr) {
		return false;
	}

	extractVolumeRegionToMesh(v, v->region(), mesh);
	return true;
}

void RawVolumeRenderer::deleteMeshes(Meshes& meshes, int idx) {
	if (meshes[idx] == nullptr) {
		return;
	}
	delete meshes[idx];
	meshes[idx] = nullptr;
	State& state = _state[idx];
	state._vertexBuffer.update(state._vertexBufferIndex, nullptr, 0);
	state._vertexBuffer.update(state._indexBufferIndex, nullptr, 0);
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
					auto i = _meshes.find(mins);
					if (i != _meshes.end()) {
						deleteMeshes(i->second, idx);
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

void RawVolumeRenderer::extractVolumeRegionToMesh(voxel::RawVolume* volume, const voxel::Region& region, voxel::Mesh* mesh) const {
	voxel::Region reg = region;
	reg.shiftUpperCorner(1, 1, 1);
	voxel::extractCubicMesh(volume, reg, mesh, voxel::IsQuadNeeded(), reg.getLowerCorner());
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

		shader::VoxelData::MaterialblockData materialBlock;
		core_memcpy(materialBlock.materialcolor, &materialColors.front(), sizeof(materialBlock.materialcolor));
		core_memcpy(materialBlock.glowcolor, &glowColors.front(), sizeof(materialBlock.glowcolor));
		_materialBlock.update(materialBlock);
	}
}

void RawVolumeRenderer::render(const video::Camera& camera, bool shadow) {
	core_trace_scoped(RawVolumeRendererRender);
	uint32_t indices[MAX_VOLUMES];

	core_memset(indices, 0, sizeof(indices));

	uint32_t numIndices = 0u;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		const State& state = _state[idx];
		if (state._hidden) {
			continue;
		}
		const uint32_t nIndices = state._vertexBuffer.elements(state._indexBufferIndex, 1, sizeof(voxel::IndexType));
		if (nIndices <= 0) {
			continue;
		}
		numIndices += nIndices;
		indices[idx] = nIndices;
	}
	if (numIndices == 0u) {
		if (_bloom->boolVal()) {
			video::ScopedFrameBuffer scoped(_frameBuffer);
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
			_shadow.render([this, &indices] (int i, const glm::mat4& lightViewProjection) {
				_shadowMapShader.setLightviewprojection(lightViewProjection);
				for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
					if (indices[idx] <= 0u) {
						continue;
					}
					const State& state = _state[idx];
					video::ScopedBuffer scopedBuf(state._vertexBuffer);
					_shadowMapShader.setModel(state._models);
					_shadowMapShader.setPivot(state._pivots);
					static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
					video::drawElementsInstanced<voxel::IndexType>(video::Primitive::Triangles, indices[idx], state._amounts);
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

	video::ScopedShader scoped(_voxelShader);
	if (_voxelShader.isDirty()) {
		_voxelShader.setMaterialblock(_materialBlock);
		if (_shadowMap->boolVal()) {
			_voxelShader.setShadowmap(video::TextureUnit::One);
		}
		_voxelShader.setDiffuseColor(_diffuseColor);
		_voxelShader.setAmbientColor(_ambientColor);
		_voxelShader.markClean();
	}
	_voxelShader.setViewprojection(camera.viewProjectionMatrix());
	if (_shadowMap->boolVal()) {
		_voxelShader.setDepthsize(glm::vec2(_shadow.dimension()));
		_voxelShader.setCascades(_shadow.cascades());
		_voxelShader.setDistances(_shadow.distances());
		_voxelShader.setLightdir(_shadow.sunDirection());
		_shadow.bind(video::TextureUnit::One);
	}

	if (_bloom->boolVal()) {
		_frameBuffer.bind(true);
	}
	const video::PolygonMode mode = camera.polygonMode();
	if (mode == video::PolygonMode::Points) {
		video::enable(video::State::PolygonOffsetPoint);
	} else if (mode == video::PolygonMode::WireFrame) {
		video::enable(video::State::PolygonOffsetLine);
	} else if (mode == video::PolygonMode::Solid) {
		video::enable(video::State::PolygonOffsetFill);
	}
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		if (indices[idx] <= 0u) {
			continue;
		}
		updatePalette(idx);
		video::ScopedPolygonMode polygonMode(mode);
		const State& state = _state[idx];
		video::ScopedBuffer scopedBuf(state._vertexBuffer);
		_voxelShader.setGray(state._gray);
		_voxelShader.setModel(state._models);
		_voxelShader.setPivot(state._pivots);
		video::drawElementsInstanced<voxel::IndexType>(video::Primitive::Triangles, indices[idx], state._amounts);
	}
	if (mode == video::PolygonMode::Points) {
		video::disable(video::State::PolygonOffsetPoint);
	} else if (mode == video::PolygonMode::WireFrame) {
		video::disable(video::State::PolygonOffsetLine);
	} else if (mode == video::PolygonMode::Solid) {
		video::disable(video::State::PolygonOffsetFill);
	}
	if (_bloom->boolVal()) {
		_frameBuffer.unbind();
		const video::TexturePtr& color0 = _frameBuffer.texture(video::FrameBufferAttachment::Color0);
		const video::TexturePtr& color1 = _frameBuffer.texture(video::FrameBufferAttachment::Color1);
		_bloomRenderer.render(color0, color1);
		video::blitFramebuffer(_frameBuffer.handle(), video::currentFramebuffer(), video::ClearFlag::Depth, _frameBuffer.dimension().x, _frameBuffer.dimension().y);
	}
}

bool RawVolumeRenderer::setInstancingAmount(int idx, int amount) {
	_state[idx]._amounts = glm::clamp(amount, 1, shader::VoxelInstancedShaderConstants::getMaxInstances());
	return amount <= shader::VoxelInstancedShaderConstants::getMaxInstances();
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
	state._amounts = reset ? 1 : state._amounts + 1;
	state._models[state._amounts - 1] = model;
	state._pivots[state._amounts - 1] = pivot;
	return true;
}

int RawVolumeRenderer::amount(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return -1;
	}
	return _state[idx]._amounts;
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
	voxel::RawVolume* old = state._rawVolume;
	if (old == volume) {
		return nullptr;
	}
	core_trace_scoped(RawVolumeRendererSetVolume);
	state._rawVolume = volume;
	state._palette.setValue(palette);
	if (deleteMesh) {
		for (auto& i : _meshes) {
			deleteMeshes(i.second, idx);
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
	_voxelShader.shutdown();
	_shadowMapShader.shutdown();
	_materialBlock.shutdown();
	_frameBuffer.shutdown();
	_bloomRenderer.shutdown();
	for (auto& iter : _meshes) {
		for (auto& mesh : iter.second) {
			delete mesh;
		}
	}
	_meshes.clear();
	core::DynamicArray<voxel::RawVolume*> old(MAX_VOLUMES);
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		State& state = _state[idx];
		state._vertexBuffer.shutdown();
		state._vertexBufferIndex = -1;
		state._indexBufferIndex = -1;
		// hand over the ownership to the caller
		old.push_back(state._rawVolume);
		state._rawVolume = nullptr;
	}
	_shadow.shutdown();
	return old;
}

}
