#include "RawVolumeRenderer.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/MaterialColor.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedPolygonMode.h"
#include "frontend/ShaderAttribute.h"

namespace frontend {

/// implementation of a function object for deciding when
/// the cubic surface extractor should insert a face between two voxels.
///
/// The criteria used here are that the voxel in front of the potential
/// quad should have a value of zero (which would typically indicate empty
/// space) while the voxel behind the potential quad would have a value
/// geater than zero (typically indicating it is solid).
struct CustomIsQuadNeeded {
	inline bool operator()(const voxel::Voxel& back, const voxel::Voxel& front, voxel::Voxel& materialToUse, voxel::FaceNames face, int x, int z) const {
		if (isBlocked(back.getMaterial()) && !isBlocked(front.getMaterial())) {
			materialToUse = back;
			return true;
		}
		return false;
	}
};

const std::string MaxDepthBufferUniformName = "u_cascades";

RawVolumeRenderer::RawVolumeRenderer(bool renderAABB, bool renderWireframe, bool renderGrid) :
		_rawVolume(nullptr), _mesh(nullptr), _shadowMapShader(shader::ShadowmapShader::getInstance()),
		_worldShader(shader::WorldShader::getInstance()), _renderAABB(renderAABB),
		_renderGrid(renderGrid), _renderWireframe(renderWireframe) {
	_sunDirection = glm::vec3(glm::left.x, glm::down.y, 0.0f);
}

bool RawVolumeRenderer::init() {
	if (!_worldShader.setup()) {
		Log::error("Failed to initialize the color shader");
		return false;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to initialize the color shader");
		return false;
	}

	if (!_shapeRenderer.init()) {
		Log::error("Failed to initialize the shape renderer");
		return false;
	}

	_vertexBufferIndex = _vertexBuffer.create();
	if (_vertexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object");
		return false;
	}

	_indexBufferIndex = _vertexBuffer.create(nullptr, 0, video::VertexBufferType::IndexBuffer);
	if (_indexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object for the indices");
		return false;
	}

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	if (!_depthBuffer.init(smSize, video::DepthBufferMode::DEPTH_CMP, maxDepthBuffers)) {
		return false;
	}

	const int shaderMaterialColorsArraySize = SDL_arraysize(shader::Materialblock::Data::materialcolor);
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
	_worldShader.setModel(glm::mat4());
	_worldShader.setTexture(video::TextureUnit::Zero);
	_worldShader.setShadowmap(video::TextureUnit::One);
	_worldShader.setFogrange(250.0f);
	_worldShader.setDiffuseColor(_diffuseColor);
	_worldShader.setAmbientColor(_ambientColor);
	_worldShader.setFogcolor(core::Color::LightBlue);

	video::Attribute attributePos = getPositionVertexAttribute(
			_vertexBufferIndex, _worldShader.getLocationPos(),
			_worldShader.getComponentsPos());
	_vertexBuffer.addAttribute(attributePos);

	video::Attribute attributeInfo = getInfoVertexAttribute(
			_vertexBufferIndex, _worldShader.getLocationInfo(),
			_worldShader.getComponentsInfo());
	_vertexBuffer.addAttribute(attributeInfo);

	if (!_shadow.init()) {
		return false;
	}

	_whiteTexture = video::createWhiteTexture("**whitetexture**");

	_mesh = new voxel::Mesh(128, 128, true);

	return true;
}

bool RawVolumeRenderer::onResize(const glm::ivec2& position, const glm::ivec2& dimension) {
	core_trace_scoped(RawVolumeRendererOnResize);
	return true;
}

bool RawVolumeRenderer::update(const std::vector<voxel::VoxelVertex>& vertices, const std::vector<voxel::IndexType>& indices) {
	core_trace_scoped(RawVolumeRendererUpdate);
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

bool RawVolumeRenderer::extract() {
	core_trace_scoped(RawVolumeRendererExtract);
	if (_rawVolume == nullptr) {
		return false;
	}

	if (_mesh == nullptr) {
		return false;
	}
	voxel::Region r = _rawVolume->getRegion();
	r.shiftUpperCorner(1, 1, 1);
	voxel::extractCubicMesh(_rawVolume, r, _mesh, CustomIsQuadNeeded());
	const size_t meshNumberIndices = _mesh->getNoOfIndices();
	if (meshNumberIndices == 0) {
		_vertexBuffer.update(_vertexBufferIndex, nullptr, 0);
		_vertexBuffer.update(_indexBufferIndex, nullptr, 0);
		return true;
	}
	return update(_mesh->getVertexVector(), _mesh->getIndexVector());
}

void RawVolumeRenderer::render(const video::Camera& camera) {
	core_trace_scoped(RawVolumeRendererRender);

	if (_renderGrid) {
		const voxel::Region& region = _rawVolume->getRegion();
		const glm::vec3& center = glm::vec3(region.getCentre());
		const glm::vec3& halfWidth = glm::vec3(region.getDimensionsInCells()) / 2.0f;
		const core::Plane planeLeft  (glm::left,     center + glm::vec3(-halfWidth.x, 0.0f, 0.0f));
		const core::Plane planeRight (glm::right,    center + glm::vec3( halfWidth.x, 0.0f, 0.0f));
		const core::Plane planeBottom(glm::down,     center + glm::vec3(0.0f, -halfWidth.y, 0.0f));
		const core::Plane planeTop   (glm::up,       center + glm::vec3(0.0f,  halfWidth.y, 0.0f));
		const core::Plane planeNear  (glm::forward,  center + glm::vec3(0.0f, 0.0f, -halfWidth.z));
		const core::Plane planeFar   (glm::backward, center + glm::vec3(0.0f, 0.0f,  halfWidth.z));

		if (planeFar.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexXYFar, camera);
		}
		if (planeNear.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexXYNear, camera);
		}

		if (planeBottom.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexXZNear, camera);
		}
		if (planeTop.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexXZFar, camera);
		}

		if (planeLeft.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexYZNear, camera);
		}
		if (planeRight.isBackSide(camera.position())) {
			_shapeRenderer.render(_gridMeshIndexYZFar, camera);
		}
	} else if (_renderAABB) {
		_shapeRenderer.render(_aabbMeshIndex, camera);
	}

	const uint32_t nIndices = _vertexBuffer.elements(_indexBufferIndex, 1, sizeof(voxel::IndexType));
	if (nIndices == 0) {
		return;
	}

	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	core_assert_always(_vertexBuffer.bind());

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	_shadow.calculateShadowData(camera, true, maxDepthBuffers, _depthBuffer.dimension());
	const std::vector<glm::mat4>& cascades = _shadow.cascades();
	const std::vector<float>& distances = _shadow.distances();
	{
		video::disable(video::State::Blend);
		// put shadow acne into the dark
		video::cullFace(video::Face::Front);
		const float shadowBiasSlope = 2;
		const float shadowBias = 0.09f;
		const float shadowRangeZ = camera.farPlane() * 3.0f;
		const glm::vec2 offset(shadowBiasSlope, (shadowBias / shadowRangeZ) * (1 << 24));
		const video::ScopedPolygonMode scopedPolygonMode(video::PolygonMode::Solid, offset);

		_depthBuffer.bind();
		video::ScopedShader scoped(_shadowMapShader);
		_shadowMapShader.setModel(glm::mat4());
		for (int i = 0; i < maxDepthBuffers; ++i) {
			_depthBuffer.bindTexture(i);
			_shadowMapShader.setLightviewprojection(cascades[i]);
			static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
			video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
		}
		_depthBuffer.unbind();
		video::cullFace(video::Face::Back);
		video::enable(video::State::Blend);
	}

	_whiteTexture->bind(video::TextureUnit::Zero);

	video::ScopedShader scoped(_worldShader);
	_worldShader.setViewprojection(camera.viewProjectionMatrix());
	_worldShader.setViewdistance(camera.farPlane());
	_worldShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
	_worldShader.setCascades(cascades);
	_worldShader.setDistances(distances);
	_worldShader.setLightdir(_shadow.sunDirection());

	video::bindTexture(video::TextureUnit::One, _depthBuffer);
	static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
	video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);

	if (_renderWireframe && camera.polygonMode() == video::PolygonMode::Solid) {
		video::ScopedPolygonMode polygonMode(video::PolygonMode::WireFrame, glm::vec2(2.0f));
		video::ScopedLineWidth lineWidth(2.0f, true);
		video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
	}

	_vertexBuffer.unbind();
	_whiteTexture->unbind();
}

voxel::RawVolume* RawVolumeRenderer::setVolume(voxel::RawVolume* volume) {
	core_trace_scoped(RawVolumeRendererSetVolume);
	voxel::RawVolume* old = _rawVolume;
	_rawVolume = volume;
	if (_rawVolume != nullptr) {
		const voxel::Region& region = _rawVolume->getRegion();
		const core::AABB<int>& intaabb = region.aabb();
		const core::AABB<float> aabb(glm::vec3(intaabb.getLowerCorner()), glm::vec3(intaabb.getUpperCorner()));
		_shapeBuilder.clear();
		_shapeBuilder.aabb(aabb, false);
		if (_aabbMeshIndex == -1) {
			_aabbMeshIndex = _shapeRenderer.createMesh(_shapeBuilder);
		} else {
			_shapeRenderer.update(_aabbMeshIndex, _shapeBuilder);
		}

		_shapeBuilder.clear();
		_shapeBuilder.aabbGridXY(aabb, false);
		if (_gridMeshIndexXYFar == -1) {
			_gridMeshIndexXYFar = _shapeRenderer.createMesh(_shapeBuilder);
		} else {
			_shapeRenderer.update(_gridMeshIndexXYFar, _shapeBuilder);
		}

		_shapeBuilder.clear();
		_shapeBuilder.aabbGridXZ(aabb, false);
		if (_gridMeshIndexXZFar == -1) {
			_gridMeshIndexXZFar = _shapeRenderer.createMesh(_shapeBuilder);
		} else {
			_shapeRenderer.update(_gridMeshIndexXZFar, _shapeBuilder);
		}

		_shapeBuilder.clear();
		_shapeBuilder.aabbGridYZ(aabb, false);
		if (_gridMeshIndexYZFar == -1) {
			_gridMeshIndexYZFar = _shapeRenderer.createMesh(_shapeBuilder);
		} else {
			_shapeRenderer.update(_gridMeshIndexYZFar, _shapeBuilder);
		}

		_shapeBuilder.clear();
		_shapeBuilder.aabbGridXY(aabb, true);
		if (_gridMeshIndexXYNear == -1) {
			_gridMeshIndexXYNear = _shapeRenderer.createMesh(_shapeBuilder);
		} else {
			_shapeRenderer.update(_gridMeshIndexXYNear, _shapeBuilder);
		}

		_shapeBuilder.clear();
		_shapeBuilder.aabbGridXZ(aabb, true);
		if (_gridMeshIndexXZNear == -1) {
			_gridMeshIndexXZNear = _shapeRenderer.createMesh(_shapeBuilder);
		} else {
			_shapeRenderer.update(_gridMeshIndexXZNear, _shapeBuilder);
		}

		_shapeBuilder.clear();
		_shapeBuilder.aabbGridYZ(aabb, true);
		if (_gridMeshIndexYZNear == -1) {
			_gridMeshIndexYZNear = _shapeRenderer.createMesh(_shapeBuilder);
		} else {
			_shapeRenderer.update(_gridMeshIndexYZNear, _shapeBuilder);
		}
	} else {
		_shapeBuilder.clear();
	}
	return old;
}

voxel::RawVolume* RawVolumeRenderer::shutdown() {
	_vertexBuffer.shutdown();
	_worldShader.shutdown();
	_shadowMapShader.shutdown();
	_materialBlock.shutdown();
	_vertexBufferIndex = -1;
	_indexBufferIndex = -1;
	_aabbMeshIndex = -1;
	_gridMeshIndexXYNear = -1;
	_gridMeshIndexXYFar = -1;
	_gridMeshIndexXZNear = -1;
	_gridMeshIndexXZFar = -1;
	_gridMeshIndexYZNear = -1;
	_gridMeshIndexYZFar = -1;
	if (_mesh != nullptr) {
		delete _mesh;
	}
	_mesh = nullptr;
	voxel::RawVolume* old = _rawVolume;
	if (_whiteTexture) {
		_whiteTexture->shutdown();
		_whiteTexture = video::TexturePtr();
	}
	_rawVolume = nullptr;
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	_depthBuffer.shutdown();
	return old;
}

size_t RawVolumeRenderer::numVertices() const {
	if (_mesh == nullptr) {
		return 0u;
	}
	return _mesh->getNoOfVertices();
}

const voxel::VoxelVertex* RawVolumeRenderer::vertices() const {
	if (_mesh == nullptr) {
		return 0u;
	}
	return _mesh->getRawVertexData();
}

size_t RawVolumeRenderer::numIndices() const {
	if (_mesh == nullptr) {
		return 0u;
	}
	return _mesh->getNoOfIndices();
}

const voxel::IndexType* RawVolumeRenderer::indices() const {
	if (_mesh == nullptr) {
		return 0u;
	}
	return _mesh->getRawIndexData();
}

}
