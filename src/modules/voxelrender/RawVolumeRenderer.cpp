/**
 * @file
 */

#include "RawVolumeRenderer.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/MaterialColor.h"
#include "video/ScopedLineWidth.h"
#include "ShaderAttribute.h"
#include "video/Camera.h"
#include "video/Types.h"
#include "video/Renderer.h"
#include "video/ScopedState.h"
#include "video/Shader.h"
#include "core/Color.h"
#include "core/Array.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "core/GameConfig.h"
#include <unordered_set>

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
		_worldShader(shader::WorldShader::getInstance()) {
}

void RawVolumeRenderer::construct() {
	core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
}

bool RawVolumeRenderer::init() {
	if (!_worldShader.setup()) {
		Log::error("Failed to initialize the world shader");
		return false;
	}

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

	const int shaderMaterialColorsArraySize = lengthof(shader::Materialblock::Data::materialcolor);
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::Materialblock::Data materialBlock;
	memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	_materialBlock.create(materialBlock);
	video::ScopedShader scoped(_worldShader);
	_worldShader.setMaterialblock(_materialBlock);
	_worldShader.setModel(glm::mat4(1.0f));
	_worldShader.setTexture(video::TextureUnit::Zero);
	_worldShader.setShadowmap(video::TextureUnit::One);
	_worldShader.setFogrange(250.0f);
	_worldShader.setDiffuseColor(_diffuseColor);
	_worldShader.setAmbientColor(_ambientColor);
	_worldShader.setFogcolor(core::Color::LightBlue);

	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		video::Attribute attributePos = getPositionVertexAttribute(
				_vertexBufferIndex[idx], _worldShader.getLocationPos(),
				_worldShader.getComponentsPos());
		_vertexBuffer[idx].addAttribute(attributePos);

		video::Attribute attributeInfo = getInfoVertexAttribute(
				_vertexBufferIndex[idx], _worldShader.getLocationInfo(),
				_worldShader.getComponentsInfo());
		_vertexBuffer[idx].addAttribute(attributeInfo);
	}

	const int maxDepthBuffers = _worldShader.getUniformArraySize(shader::WorldShader::getMaxDepthBufferUniformName());
	if (!_shadow.init(maxDepthBuffers)) {
		return false;
	}

	_whiteTexture = video::createWhiteTexture("**whitetexture**");

	_meshSize = core::Var::getSafe(cfg::VoxelMeshSize);

	return true;
}

bool RawVolumeRenderer::update(int idx) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	core_trace_scoped(RawVolumeRendererUpdate);
	std::vector<voxel::VoxelVertex> vertices;
	std::vector<voxel::IndexType> indices;

	voxel::IndexType offset = (voxel::IndexType)0;
	for (auto& i : _meshes) {
		const Meshes& meshes = i.second;
		const voxel::Mesh* mesh = meshes[idx];
		if (mesh == nullptr || mesh->getNoOfIndices() <= 0) {
			continue;
		}
		const std::vector<voxel::VoxelVertex>& vertexVector = mesh->getVertexVector();
		const std::vector<voxel::IndexType>& indexVector = mesh->getIndexVector();
		std::copy(vertexVector.begin(), vertexVector.end(), std::back_inserter(vertices));

		for (const auto& iv : indexVector) {
			indices.push_back(iv + offset);
		}

		offset += vertexVector.size();
	}

	return update(idx, vertices, indices);
}

bool RawVolumeRenderer::update(int idx, const std::vector<voxel::VoxelVertex>& vertices, const std::vector<voxel::IndexType>& indices) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	core_trace_scoped(RawVolumeRendererUpdate);

	if (indices.empty()) {
		_vertexBuffer[idx].update(_vertexBufferIndex[idx], nullptr, 0);
		_vertexBuffer[idx].update(_indexBufferIndex[idx], nullptr, 0);
		return true;
	}

	if (!_vertexBuffer[idx].update(_vertexBufferIndex[idx], vertices)) {
		Log::error("Failed to update the vertex buffer");
		return false;
	}
	if (!_vertexBuffer[idx].update(_indexBufferIndex[idx], indices)) {
		Log::error("Failed to update the index buffer");
		return false;
	}
	return true;
}

bool RawVolumeRenderer::empty(int idx) const {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return true;
	}
	for (auto& i : _meshes) {
		const Meshes& meshes = i.second;
		if (meshes[idx] != nullptr && meshes[idx]->getNoOfIndices() > 0) {
			return true;
		}
	}
	return false;
}

bool RawVolumeRenderer::toMesh(int idx, voxel::Mesh* mesh) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	voxel::RawVolume* volume = _rawVolume[idx];
	if (volume == nullptr) {
		return false;
	}

	extract(volume, volume->region(), mesh);
	return true;
}

bool RawVolumeRenderer::extract(int idx, const voxel::Region& region) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}
	voxel::RawVolume* volume = _rawVolume[idx];
	if (volume == nullptr) {
		return false;
	}

	const int s = _meshSize->intVal();
	glm::ivec3 meshSize(s, s, s);
	const glm::vec3& size = meshSize;

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

	for (int cx = lowerX; cx <= upperX; ++cx) {
		if (!completeRegion.containsPointInX(cx)) {
			continue;
		}
		const int x = glm::floor(cx / size.x);
		for (int cy = lowerY; cy <= upperY; ++cy) {
			if (!completeRegion.containsPointInY(cy)) {
				continue;
			}
			const int y = glm::floor(cy / size.y);
			for (int cz = lowerZ; cz <= upperZ; ++cz) {
				if (!completeRegion.containsPointInZ(cz)) {
					continue;
				}
				const int z = glm::floor(cz / size.z);
				const glm::ivec3 mins(x * meshSize.x, y * meshSize.y, z * meshSize.z);

				auto i = extracted.insert(mins);
				if (!i.second) {
					continue;
				}

				const glm::ivec3 maxs = mins + meshSize - 1;

				Meshes& meshes = _meshes[mins];
				if (meshes[idx] == nullptr) {
					meshes[idx] = new voxel::Mesh(128, 128, true);
				}
				extract(volume, voxel::Region{mins, maxs}, meshes[idx]);
			}
		}
	}
	if (!update(idx)) {
		Log::error("Failed to update the mesh at index %i", idx);
	}
	return true;
}

void RawVolumeRenderer::extract(voxel::RawVolume* volume, const voxel::Region& region, voxel::Mesh* mesh) const {
	voxel::Region reg = region;
	reg.shiftUpperCorner(1, 1, 1);
	voxel::extractCubicMesh(volume, reg, mesh, raw::CustomIsQuadNeeded());
}

void RawVolumeRenderer::render(const video::Camera& camera, bool shadow) {
	core_trace_scoped(RawVolumeRendererRender);

	uint32_t numIndices = 0u;
	for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
		numIndices += _vertexBuffer[idx].elements(_indexBufferIndex[idx], 1, sizeof(voxel::IndexType));
		if (numIndices > 0) {
			break;
		}
	}
	if (numIndices == 0u) {
		return;
	}

	video::ScopedState scopedDepth(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::ScopedState scopedCullFace(video::State::CullFace);
	video::ScopedState scopedDepthMask(video::State::DepthMask);

	_shadow.update(camera, shadow);
	if (shadow) {
		_shadow.render([this] (int i, shader::ShadowmapShader& shader) {
			for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
				const uint32_t nIndices = _vertexBuffer[idx].elements(_indexBufferIndex[idx], 1, sizeof(voxel::IndexType));
				if (nIndices == 0) {
					continue;
				}
				video::ScopedBuffer scopedBuf(_vertexBuffer[idx]);
				shader.setModel(_model[idx]);
				static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
				video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
			}
			return true;
		}, [] (int, shader::ShadowmapInstancedShader&) {return true;});
	}

	{
		video::ScopedTexture scopedTex(_whiteTexture, video::TextureUnit::Zero);
		video::ScopedShader scoped(_worldShader);
		_worldShader.setViewprojection(camera.viewProjectionMatrix());
		_worldShader.setViewdistance(camera.farPlane());
		_worldShader.setDepthsize(glm::vec2(_shadow.dimension()));
		_worldShader.setCascades(_shadow.cascades());
		_worldShader.setDistances(_shadow.distances());
		_worldShader.setLightdir(_shadow.sunDirection());

		video::ScopedPolygonMode polygonMode(camera.polygonMode());
		_shadow.bind(video::TextureUnit::One);
		for (int idx = 0; idx < MAX_VOLUMES; ++idx) {
			const uint32_t nIndices = _vertexBuffer[idx].elements(_indexBufferIndex[idx], 1, sizeof(voxel::IndexType));
			if (nIndices == 0) {
				continue;
			}
			video::ScopedBuffer scopedBuf(_vertexBuffer[idx]);
			_worldShader.setModel(_model[idx]);
			static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
			video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
		}
	}
}

bool RawVolumeRenderer::setModelMatrix(int idx, const glm::mat4& model) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return false;
	}

	_model[idx] = model;

	return true;
}

voxel::RawVolume* RawVolumeRenderer::setVolume(int idx, voxel::RawVolume* volume) {
	if (idx < 0 || idx >= MAX_VOLUMES) {
		return nullptr;
	}
	core_trace_scoped(RawVolumeRendererSetVolume);

	voxel::RawVolume* old = _rawVolume[idx];
	_rawVolume[idx] = volume;

	return old;
}

std::vector<voxel::RawVolume*> RawVolumeRenderer::shutdown() {
	_worldShader.shutdown();
	_materialBlock.shutdown();
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
		for (auto i : _meshes) {
			for (auto &mesh : i.second) {
				delete mesh;
				mesh = nullptr;
			}
		}
		old.push_back(_rawVolume[idx]);
		_rawVolume[idx] = nullptr;
	}
	if (_whiteTexture) {
		_whiteTexture->shutdown();
		_whiteTexture = video::TexturePtr();
	}
	_shadow.shutdown();
	return old;
}

}
