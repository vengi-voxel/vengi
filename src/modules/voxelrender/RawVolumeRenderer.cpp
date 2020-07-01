/**
 * @file
 */

#include "RawVolumeRenderer.h"
#include "core/Trace.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxelutil/VolumeMerger.h"
#include "voxel/MaterialColor.h"
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
#include "core/StandardLib.h"
#include "VoxelShaderConstants.h"
#include <SDL.h>
#include <unordered_set>
#include <algorithm>

namespace voxelrender {

namespace raw {
/// implementation of a function object for deciding when
/// the cubic surface extractor should insert a face between two voxels.
///
/// The criteria used here are that the voxel in front of the potential
/// quad should have a value of zero (which would typically indicate empty
/// space) while the voxel behind the potential quad would have a value
/// greater than zero (typically indicating it is solid).
struct CustomIsQuadNeeded {
	inline bool operator()(const voxel::VoxelType& back, const voxel::VoxelType& front, voxel::FaceNames face) const {
		if (isBlocked(back) && !isBlocked(front)) {
			return true;
		}
		return false;
	}
};
}

RawVolumeRenderer::RawVolumeRenderer() :
		_voxelShader(shader::VoxelShader::getInstance()),
		_shadowMapShader(shader::ShadowmapShader::getInstance()) {
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
	_shadowMap = core::Var::getSafe(cfg::ClientShadowMap);

	_threadPool.init();

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		_model[idx] = glm::mat4(1.0f);
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

	const int shaderMaterialColorsArraySize = lengthof(shader::VoxelData::MaterialblockData::materialcolor);
	const int materialColorsArraySize = voxel::getMaterialColors().size();
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

	if (!_indirectDrawBuffer.init()) {
		return false;
	}

	render::ShadowParameters shadowParams;
	shadowParams.maxDepthBuffers = shader::VoxelShaderConstants::getMaxDepthBuffers();
	if (!_shadow.init(shadowParams)) {
		return false;
	}

	shader::VoxelData::MaterialblockData materialBlock;
	core_memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
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
		meshes[result.idx] = new voxel::Mesh(std::move(result.mesh));
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
	voxel::VertexArray vertices;
	voxel::IndexArray indices;

	voxel::IndexType offset = (voxel::IndexType)0;
	for (auto& i : _meshes) {
		const Meshes& meshes = i.second;
		const voxel::Mesh* mesh = meshes[idx];
		if (mesh == nullptr || mesh->getNoOfIndices() <= 0) {
			continue;
		}
		const voxel::VertexArray& vertexVector = mesh->getVertexVector();
		const voxel::IndexArray& indexVector = mesh->getIndexVector();
		std::copy(vertexVector.begin(), vertexVector.end(), std::back_inserter(vertices));

		for (const auto& iv : indexVector) {
			indices.push_back(iv + offset);
		}

		offset += vertexVector.size();
	}

	return update(idx, vertices, indices);
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

bool RawVolumeRenderer::swap(int idx1, int idx2) {
	if (idx1 < 0 || idx1 >= MAX_VOLUMES) {
		return false;
	}
	if (idx2 < 0 || idx2 >= MAX_VOLUMES) {
		return false;
	}
	if (idx1 == idx2) {
		return true;
	}
	for (auto& i : _meshes) {
		Meshes& meshes = i.second;
		std::swap(meshes[idx1], meshes[idx2]);
	}
	std::swap(_hidden[idx1], _hidden[idx2]);
	std::swap(_model[idx1], _model[idx2]);
	std::swap(_rawVolume[idx1], _rawVolume[idx2]);
	update(idx1);
	update(idx2);

	return true;
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
	std::vector<const voxel::RawVolume*> volumes;
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
	const glm::vec3 meshSizef = meshSize;

	const glm::ivec3& lower = region.getLowerCorner();
	const glm::ivec3& upper = region.getUpperCorner();

	const int border = 1;
	const voxel::Region& completeRegion = volume->region();
	const int xGap = lower.x % meshSize.x;
	const int yGap = lower.y % meshSize.y;
	const int zGap = lower.z % meshSize.z;
	const int lowerX = lower.x - ((xGap == 0) ? border : 0);
	const int lowerY = lower.y - ((yGap == 0) ? border : 0);
	const int lowerZ = lower.z - ((zGap == 0) ? border : 0);

	const int upperX = upper.x + ((xGap == meshSize.x - 1) ? border : 0);
	const int upperY = upper.y + ((yGap == meshSize.y - 1) ? border : 0);
	const int upperZ = upper.z + ((zGap == meshSize.z - 1) ? border : 0);

	std::unordered_set<glm::ivec3, std::hash<glm::ivec3> > extracted;

	// TODO: this is iteration over a lot of duplicated region points
	for (int cx = lowerX; cx <= upperX; ++cx) {
		const int x = glm::floor(cx / meshSizef.x);
		for (int cy = lowerY; cy <= upperY; ++cy) {
			const int y = glm::floor(cy / meshSizef.y);
			for (int cz = lowerZ; cz <= upperZ; ++cz) {
				const int z = glm::floor(cz / meshSizef.z);
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

				auto i = extracted.insert(mins);
				if (!i.second) {
					continue;
				}

				voxel::RawVolume copy(volume);
				_threadPool.enqueue([movedCopy = std::move(copy), mins, idx, finalRegion, this] () {
					++_runningExtractorTasks;
					voxel::Region reg = finalRegion;
					reg.shiftUpperCorner(1, 1, 1);
					voxel::Mesh mesh(65536, 65536, true);
					voxel::extractCubicMesh(&movedCopy, reg, &mesh, raw::CustomIsQuadNeeded(), reg.getLowerCorner());
					_pendingQueue.emplace(mins, idx, std::move(mesh));
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
	voxel::extractCubicMesh(volume, reg, mesh, raw::CustomIsQuadNeeded(), reg.getLowerCorner());
}

bool RawVolumeRenderer::hiddenState(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return true;
	}
	return _hidden[idx];
}

void RawVolumeRenderer::hide(int idx, bool hide) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return;
	}
	_hidden[idx] = hide;
}

void RawVolumeRenderer::render(const video::Camera& camera, bool shadow) {
	core_trace_scoped(RawVolumeRendererRender);

	if (voxel::materialColorChanged()) {
		shader::VoxelData::MaterialblockData materialBlock;
		core_memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
		_materialBlock.update(materialBlock);
		// TODO: updating the global state is crap - what about others - use an event
		voxel::materialColorMarkClean();
	}

	core_memset(_drawCommands, 0, sizeof(_drawCommands));

	uint32_t numIndices = 0u;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		if (_hidden[idx]) {
			continue;
		}
		const uint32_t nIndices = _vertexBuffer[idx].elements(_indexBufferIndex[idx], 1, sizeof(voxel::IndexType));
		if (nIndices <= 0) {
			continue;
		}
		numIndices += nIndices;
		_drawCommands[idx].count = nIndices;
		_drawCommands[idx].instanceCount = 1;
	}
	if (numIndices == 0u) {
		return;
	}
	core_assert_always(_indirectDrawBuffer.update(_drawCommands, sizeof(_drawCommands)));

	video::ScopedState scopedDepth(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::ScopedState scopedCullFace(video::State::CullFace);
	video::ScopedState scopedDepthMask(video::State::DepthMask);
	_indirectDrawBuffer.bind();
	if (_shadowMap->boolVal()) {
		_shadow.update(camera, true);
		if (shadow) {
			video::ScopedShader scoped(_shadowMapShader);
			_shadow.render([this] (int i, const glm::mat4& lightViewProjection) {
				_shadowMapShader.setLightviewprojection(lightViewProjection);
				for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
					if (_drawCommands[idx].count <= 0u) {
						continue;
					}
					video::ScopedBuffer scopedBuf(_vertexBuffer[idx]);
					_shadowMapShader.setModel(_model[idx]);
					static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
					void* bufferOffset = (void*)(intptr_t)(idx * sizeof(_drawCommands[0]));
					video::drawElementsIndirect<voxel::IndexType>(video::Primitive::Triangles, bufferOffset);
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

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		if (_drawCommands[idx].count <= 0u) {
			continue;
		}
		const glm::vec2 offset(-0.25f * idx, -0.5f * idx);
		video::ScopedPolygonMode polygonMode(camera.polygonMode(), offset);
		video::ScopedBuffer scopedBuf(_vertexBuffer[idx]);
		_voxelShader.setModel(_model[idx]);
		void* bufferOffset = (void*)(intptr_t)(idx * sizeof(_drawCommands[0]));
		video::drawElementsIndirect<voxel::IndexType>(video::Primitive::Triangles, bufferOffset);
	}
	_indirectDrawBuffer.unbind();
}

bool RawVolumeRenderer::setModelMatrix(int idx, const glm::mat4& model) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}

	_model[idx] = model;

	return true;
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

std::vector<voxel::RawVolume*> RawVolumeRenderer::shutdown() {
	_threadPool.shutdown();
	_voxelShader.shutdown();
	_shadowMapShader.shutdown();
	_materialBlock.shutdown();
	_indirectDrawBuffer.shutdown();
	for (auto& iter : _meshes) {
		for (auto& mesh : iter.second) {
			delete mesh;
		}
	}
	_meshes.clear();
	std::vector<voxel::RawVolume*> old(MAX_VOLUMES);
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
