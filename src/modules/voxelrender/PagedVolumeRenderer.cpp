/**
 * @file
 */

#include "PagedVolumeRenderer.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/MaterialColor.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedState.h"
#include "ShaderAttribute.h"
#include "video/Camera.h"
#include "video/Types.h"
#include "video/Renderer.h"
#include "video/Shader.h"
#include "core/Color.h"
#include "core/Array.h"
#include "core/GLM.h"
#include "core/Var.h"
#include "core/GameConfig.h"

namespace voxelrender {

namespace paged {
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

PagedVolumeRenderer::PagedVolumeRenderer() :
		_worldShader(shader::WorldShader::getInstance()) {
}

bool PagedVolumeRenderer::init() {
	if (!_worldShader.setup()) {
		Log::error("Failed to initialize the world shader");
		return false;
	}

	_vertexBufferIndex = _vertexBuffer.create();
	if (_vertexBufferIndex == -1) {
	Log::error("Could not create the vertex buffer object");
		return false;
	}

	_indexBufferIndex = _vertexBuffer.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (_indexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object for the indices");
		return false;
	}

	const int shaderMaterialColorsArraySize = lengthof(shader::WorldData::MaterialblockData::materialcolor);
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::WorldData::MaterialblockData materialBlock;
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

	const video::Attribute& attributePos = getPositionVertexAttribute(
			_vertexBufferIndex, _worldShader.getLocationPos(),
			_worldShader.getComponentsPos());
	_vertexBuffer.addAttribute(attributePos);

	const video::Attribute& attributeInfo = getInfoVertexAttribute(
			_vertexBufferIndex, _worldShader.getLocationInfo(),
			_worldShader.getComponentsInfo());
	_vertexBuffer.addAttribute(attributeInfo);

	const int maxDepthBuffers = _worldShader.getUniformArraySize(shader::WorldShader::getMaxDepthBufferUniformName());
	if (!_shadow.init(maxDepthBuffers)) {
		return false;
	}

	_whiteTexture = video::createWhiteTexture("**whitetexture**");
	_mesh = new voxel::Mesh(128, 128, true);

	return true;
}

bool PagedVolumeRenderer::onResize(const glm::ivec2& position, const glm::ivec2& dimension) {
	core_trace_scoped(PagedVolumeRendererOnResize);
	return true;
}

bool PagedVolumeRenderer::update(const std::vector<voxel::VoxelVertex>& vertices, const std::vector<voxel::IndexType>& indices) {
	core_trace_scoped(PagedVolumeRendererUpdate);
	if (!_vertexBuffer.update(_vertexBufferIndex, vertices)) {
		Log::error("Failed to update the vertex buffer");
		return false;
	}
	if (!_vertexBuffer.update(_indexBufferIndex, indices)) {
		Log::error("Failed to update the index buffer");
		return false;
	}
	return true;
}

voxel::PagedVolume* PagedVolumeRenderer::setVolume(voxel::PagedVolume* volume) {
	voxel::PagedVolume* old = _volume;
	_volume = volume;
	return old;
}

bool PagedVolumeRenderer::extract() {
	voxel::PagedVolume* volume = _volume;
	if (volume == nullptr) {
		return false;
	}

	if (_mesh == nullptr) {
		return false;
	}

	voxel::Region region = _volume->region();
	region.shiftUpperCorner(1, 1, 1);
	voxel::extractCubicMesh(_volume, region, _mesh, paged::CustomIsQuadNeeded());

	update();
	return true;
}

bool PagedVolumeRenderer::update() {
	const size_t meshNumberIndices = _mesh->getNoOfIndices();
	if (meshNumberIndices == 0u) {
		_vertexBuffer.update(_vertexBufferIndex, nullptr, 0);
		_vertexBuffer.update(_indexBufferIndex, nullptr, 0);
		return true;
	}
	return update(_mesh->getVertexVector(), _mesh->getIndexVector());
}

void PagedVolumeRenderer::render(const video::Camera& camera) {
	core_trace_scoped(PagedVolumeRendererRender);

	uint32_t numIndices = _vertexBuffer.elements(_indexBufferIndex, 1, sizeof(voxel::IndexType));
	if (numIndices == 0u) {
		return;
	}

	video::ScopedState scopedDepth(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::ScopedState scopedCullFace(video::State::CullFace);
	video::ScopedState scopedDepthMask(video::State::DepthMask);

	_shadow.update(camera, true);
	_shadow.render([this] (int i, shader::ShadowmapShader& shader) {
		const uint32_t nIndices = _vertexBuffer.elements(_indexBufferIndex, 1, sizeof(voxel::IndexType));
		video::ScopedBuffer scopedBuf(_vertexBuffer);
		shader.setModel(glm::mat4());
		static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
		video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
		return true;
	}, [] (int, shader::ShadowmapInstancedShader&) {return true;});

	video::ScopedTexture scopedTex(_whiteTexture, video::TextureUnit::Zero);
	video::ScopedShader scoped(_worldShader);
	_worldShader.setViewprojection(camera.viewProjectionMatrix());
	_worldShader.setFocuspos(camera.target());
	_worldShader.setDepthsize(glm::vec2(_shadow.dimension()));
	_worldShader.setCascades(_shadow.cascades());
	_worldShader.setDistances(_shadow.distances());
	_worldShader.setLightdir(_shadow.sunDirection());

	video::ScopedPolygonMode polygonMode(camera.polygonMode());
	_shadow.bind(video::TextureUnit::One);
	const uint32_t nIndices = _vertexBuffer.elements(_indexBufferIndex, 1, sizeof(voxel::IndexType));
	video::ScopedBuffer scopedBuf(_vertexBuffer);
	_worldShader.setModel(glm::mat4());
	static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
	video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
}

void PagedVolumeRenderer::shutdown() {
	_worldShader.shutdown();
	_materialBlock.shutdown();
	_vertexBuffer.shutdown();
	_vertexBufferIndex = -1;
	_indexBufferIndex = -1;
	delete _mesh;
	_mesh = nullptr;
	delete _volume;
	_volume = nullptr;
	if (_whiteTexture) {
		_whiteTexture->shutdown();
		_whiteTexture = video::TexturePtr();
	}
	_shadow.shutdown();
}

}
