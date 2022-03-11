/**
 * @file
 */

#include "RawVolumeRenderer.h"
#include "core/Common.h"
#include "core/Trace.h"
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

bool RawVolumeRenderer::init() {
	if (!_voxelShader.setup()) {
		Log::error("Failed to initialize the world shader");
		return false;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to init shadowmap shader");
		return false;
	}

	video::FrameBufferConfig cfg;
	const glm::ivec2 &windowSize = video::getWindowSize();
	cfg.dimension(windowSize);
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color0); // scene
	cfg.addTextureAttachment(video::createDefaultTextureConfig(), video::FrameBufferAttachment::Color1); // bloom
	cfg.depthBuffer(true);
	if (!_frameBuffer.init(cfg)) {
		Log::error("Failed to initialize the volume renderer framebuffer");
		return false;
	}

	// we have to do an y-flip here due to the framebuffer handling
	if (!_bloomRenderer.init(true, windowSize.x, windowSize.y)) {
		Log::error("Failed to initialize the blur renderer");
		return false;
	}

	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);
	_bloom = core::Var::getSafe(cfg::ClientBloom);

	_threadPool.init();

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		_models[idx][0] = glm::mat4(1.0f);
		_amounts[idx] = 1;
		_vertexBufferIndex[idx] = _vertexBuffer[idx].create();
		if (_vertexBufferIndex[idx] == -1) {
			Log::error("Could not create the vertex buffer object");
			return false;
		}

		_indexBufferIndex[idx] = _vertexBuffer[idx].create(nullptr, 0, video::BufferType::IndexBuffer);
		if (_indexBufferIndex[idx] == -1) {
			Log::error("Could not create the vertex buffer object for the indices");
			return false;
		}
	}

	const voxel::Palette &palette = voxel::getPalette();
	core::DynamicArray<glm::vec4> materialColors;
	palette.toVec4f(materialColors);
	core::DynamicArray<glm::vec4> glowColors;
	palette.glowToVec4f(glowColors);

	const int shaderMaterialColorsArraySize = lengthof(shader::VoxelData::MaterialblockData::materialcolor);
	const int materialColorsArraySize = (int)materialColors.size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		const video::Attribute& attributePos = getPositionVertexAttribute(
				_vertexBufferIndex[idx], _voxelShader.getLocationPos(),
				_voxelShader.getComponentsPos());
		_vertexBuffer[idx].addAttribute(attributePos);

		const video::Attribute& attributeInfo = getInfoVertexAttribute(
				_vertexBufferIndex[idx], _voxelShader.getLocationInfo(),
				_voxelShader.getComponentsInfo());
		_vertexBuffer[idx].addAttribute(attributeInfo);
	}

	render::ShadowParameters shadowParams;
	shadowParams.maxDepthBuffers = shader::VoxelShaderConstants::getMaxDepthBuffers();
	if (!_shadow.init(shadowParams)) {
		Log::error("Failed to initialize the shadow object");
		return false;
	}

	shader::VoxelData::MaterialblockData materialBlock;
	core_memcpy(materialBlock.materialcolor, &materialColors.front(), sizeof(materialBlock.materialcolor));
	core_memcpy(materialBlock.glowcolor, &glowColors.front(), sizeof(materialBlock.glowcolor));
	_materialBlock.create(materialBlock);

	_meshSize = core::Var::getSafe(cfg::VoxelMeshSize);

	return true;
}

void RawVolumeRenderer::update() {
	ExtractionCtx result;
	int cnt = 0;
	while (_pendingQueue.pop(result)) {
		Meshes& meshes = _meshes[result.mins];
		if (meshes[result.idx] != nullptr) {
			delete meshes[result.idx];
		}
		meshes[result.idx] = new voxel::Mesh(core::move(result.mesh));
		if (!update(result.idx)) {
			Log::error("Failed to update the mesh at index %i", result.idx);
		}
		++cnt;
	}
	if (cnt > 0) {
		Log::debug("Perform %i mesh updates in this frame", cnt);
	}
}

bool RawVolumeRenderer::update(int idx) {
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

	if (indCount == 0u || vertCount == 0u) {
		_vertexBuffer[idx].update(_vertexBufferIndex[idx], nullptr, 0);
		_vertexBuffer[idx].update(_indexBufferIndex[idx], nullptr, 0);
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

	if (!_vertexBuffer[idx].update(_vertexBufferIndex[idx], verticesBuf, verticesBufSize)) {
		Log::error("Failed to update the vertex buffer");
		core_free(indicesBuf);
		core_free(verticesBuf);
		return false;
	}
	core_free(verticesBuf);

	if (!_vertexBuffer[idx].update(_indexBufferIndex[idx], indicesBuf, indicesBufSize)) {
		Log::error("Failed to update the index buffer");
		core_free(indicesBuf);
		return false;
	}
	core_free(indicesBuf);
	return true;
}

bool RawVolumeRenderer::update(int idx, const voxel::VertexArray& vertices, const voxel::IndexArray& indices) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	core_trace_scoped(RawVolumeRendererUpdate);

	if (indices.empty() || vertices.empty()) {
		_vertexBuffer[idx].update(_vertexBufferIndex[idx], nullptr, 0);
		_vertexBuffer[idx].update(_indexBufferIndex[idx], nullptr, 0);
		return true;
	}

	if (!_vertexBuffer[idx].update(_vertexBufferIndex[idx], &vertices.front(), vertices.size() * sizeof(voxel::VertexArray::value_type))) {
		Log::error("Failed to update the vertex buffer");
		return false;
	}
	if (!_vertexBuffer[idx].update(_indexBufferIndex[idx], &indices.front(), indices.size() * sizeof(voxel::IndexArray::value_type))) {
		Log::error("Failed to update the index buffer");
		return false;
	}
	return true;
}

void RawVolumeRenderer::setAmbientColor(const glm::vec3& color) {
	_ambientColor = color;
	// force updating the cached uniform values
	_voxelShader.markDirty();
}

void RawVolumeRenderer::setDiffuseColor(const glm::vec3& color) {
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

bool RawVolumeRenderer::toMesh(voxel::Mesh* mesh) {
	core::DynamicArray<const voxel::RawVolume*> volumes;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		const voxel::RawVolume* volume = _rawVolume[idx];
		if (volume == nullptr) {
			continue;
		}
		volumes.push_back(volume);
	}
	if (volumes.empty()) {
		return false;
	}

	voxel::RawVolume* mergedVolume = merge(volumes);
	if (mergedVolume == nullptr) {
		return false;
	}
	extractVolumeRegionToMesh(mergedVolume, mergedVolume->region(), mesh);
	delete mergedVolume;
	return true;
}

bool RawVolumeRenderer::toMesh(int idx, voxel::Mesh* mesh) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	voxel::RawVolume* volume = _rawVolume[idx];
	if (volume == nullptr) {
		return false;
	}

	extractVolumeRegionToMesh(volume, volume->region(), mesh);
	return true;
}

bool RawVolumeRenderer::translate(int idx, const glm::ivec3& m) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	voxel::RawVolume* volume = _rawVolume[idx];
	if (volume == nullptr) {
		return false;
	}
	volume->translate(m);
	for (auto& i : _meshes) {
		Meshes& meshes = i.second;
		if (meshes[idx] == nullptr) {
			continue;
		}
		delete meshes[idx];
		meshes[idx] = nullptr;
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
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	voxel::RawVolume* volume = _rawVolume[idx];
	if (volume == nullptr) {
		return false;
	}

	const int s = _meshSize->intVal();
	const glm::ivec3 meshSize(s);
	const glm::ivec3 meshSizeMinusOne(s - 1);
	const voxel::Region& completeRegion = volume->region();

	// convert to step coordinates that are needed to extract
	// the given region mesh size ranges
	const glm::ivec3& l = (region.getLowerCorner() - meshSizeMinusOne) / meshSize;
	const glm::ivec3& u = region.getUpperCorner() / meshSize;

	for (int x = l.x; x <= u.x; ++x) {
		for (int y = l.y; y <= u.y; ++y) {
			for (int z = l.z; z <= u.z; ++z) {
				const voxel::Region& finalRegion = calculateExtractRegion(x, y, z, meshSize);
				const glm::ivec3& mins = finalRegion.getLowerCorner();

				if (!voxel::intersects(completeRegion, finalRegion)) {
					auto i = _meshes.find(mins);
					if (i != _meshes.end()) {
						Meshes& meshes = i->second;
						delete meshes[idx];
						meshes[idx] = nullptr;
					}
					continue;
				}

				voxel::RawVolume copy(volume);
				_threadPool.enqueue([movedCopy = core::move(copy), mins, idx, finalRegion, this] () {
					++_runningExtractorTasks;
					voxel::Region reg = finalRegion;
					reg.shiftUpperCorner(1, 1, 1);
					voxel::Mesh mesh(65536, 65536, true);
					voxel::extractCubicMesh(&movedCopy, reg, &mesh, voxel::IsQuadNeeded(), reg.getLowerCorner());
					_pendingQueue.emplace(mins, idx, core::move(mesh));
					Log::debug("Enqueue mesh for idx: %i", idx);
					--_runningExtractorTasks;
				});
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

void RawVolumeRenderer::render(const video::Camera& camera, bool shadow) {
	core_trace_scoped(RawVolumeRendererRender);

	voxel::Palette &palette = voxel::getPalette();
	if (palette.isDirty()) {
		core::DynamicArray<glm::vec4> materialColors;
		palette.toVec4f(materialColors);
		core::DynamicArray<glm::vec4> glowColors;
		palette.glowToVec4f(glowColors);

		shader::VoxelData::MaterialblockData materialBlock;
		core_memcpy(materialBlock.materialcolor, &materialColors.front(), sizeof(materialBlock.materialcolor));
		core_memcpy(materialBlock.glowcolor, &glowColors.front(), sizeof(materialBlock.glowcolor));
		_materialBlock.update(materialBlock);
		// TODO: updating the global state is crap - what about others - use an event
		palette.markClean();
	}

	renderVolumes(camera, shadow);
}

void RawVolumeRenderer::renderVolumes(const video::Camera& camera, bool shadow) {
	uint32_t indices[MAX_VOLUMES];

	core_memset(indices, 0, sizeof(indices));

	uint32_t numIndices = 0u;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		if (_state[idx]._hidden) {
			continue;
		}
		const uint32_t nIndices = _vertexBuffer[idx].elements(_indexBufferIndex[idx], 1, sizeof(voxel::IndexType));
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
		} else {
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
					video::ScopedBuffer scopedBuf(_vertexBuffer[idx]);
					_shadowMapShader.setModel(_models[idx]);
					_shadowMapShader.setPivot(_pivots[idx]);
					static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
					video::drawElementsInstanced<voxel::IndexType>(video::Primitive::Triangles, indices[idx], _amounts[idx]);
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
		video::ScopedPolygonMode polygonMode(mode);
		video::ScopedBuffer scopedBuf(_vertexBuffer[idx]);
		_voxelShader.setGray(_state[idx]._gray);
		_voxelShader.setModel(_models[idx]);
		_voxelShader.setPivot(_pivots[idx]);
		video::drawElementsInstanced<voxel::IndexType>(video::Primitive::Triangles, indices[idx], _amounts[idx]);
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
	}
}

void RawVolumeRenderer::setInstancingAmount(int idx, int amount) {
	_amounts[idx] = amount;
}

bool RawVolumeRenderer::setModelMatrix(int idx, const glm::mat4& model, const glm::vec3 &pivot, bool reset) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		Log::error("Given id %i is out of bounds", idx);
		return false;
	}
	if (_rawVolume[idx] == nullptr) {
		Log::error("No volume found at: %i", idx);
		return false;
	}
	_amounts[idx] = reset ? 1 : _amounts[idx] + 1;
	_models[idx][_amounts[idx] - 1] = model;
	_pivots[idx][_amounts[idx] - 1] = pivot;
	return true;
}

int RawVolumeRenderer::amount(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return -1;
	}
	return _amounts[idx];
}

voxel::Region RawVolumeRenderer::region() const {
	voxel::Region region;
	bool validVolume = false;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		voxel::RawVolume* volume = _rawVolume[idx];
		if (volume == nullptr) {
			continue;
		}
		if (validVolume) {
			region.accumulate(volume->region());
			continue;
		}
		region = volume->region();
		validVolume = true;
	}
	return region;
}

voxel::RawVolume* RawVolumeRenderer::setVolume(int idx, voxel::SceneGraphNode& node, bool deleteMesh) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	core_trace_scoped(RawVolumeRendererSetVolume);

	voxel::RawVolume* old = _rawVolume[idx];
	_rawVolume[idx] = node.volume();
	if (deleteMesh) {
		for (auto& i : _meshes) {
			Meshes& meshes = i.second;
			delete meshes[idx];
			meshes[idx] = nullptr;
		}
	}
	return old;
}

voxel::RawVolume* RawVolumeRenderer::setVolume(int idx, voxel::RawVolume* volume, bool deleteMesh) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	core_trace_scoped(RawVolumeRendererSetVolume);

	voxel::RawVolume* old = _rawVolume[idx];
	_rawVolume[idx] = volume;
	if (deleteMesh) {
		for (auto& i : _meshes) {
			Meshes& meshes = i.second;
			delete meshes[idx];
			meshes[idx] = nullptr;
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
		_vertexBuffer[idx].shutdown();
		_vertexBufferIndex[idx] = -1;
		_indexBufferIndex[idx] = -1;
		// hand over the ownership to the caller
		old.push_back(_rawVolume[idx]);
		_rawVolume[idx] = nullptr;
	}
	_shadow.shutdown();
	return old;
}

}
