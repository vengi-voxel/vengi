#include "RawVolumeRenderer.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/MaterialColor.h"
#include "video/ScopedLineWidth.h"
#include "video/ScopedPolygonMode.h"

namespace frontend {

const std::string MaxDepthBufferUniformName = "u_farplanes";

RawVolumeRenderer::RawVolumeRenderer(bool renderAABB, bool renderWireframe, bool renderGrid) :
		_rawVolume(nullptr), _mesh(nullptr),
		_worldShader(shader::WorldShader::getInstance()), _renderAABB(renderAABB),
		_renderGrid(renderGrid), _renderWireframe(renderWireframe) {
	_sunDirection = glm::vec3(glm::left.x, glm::down.y, 0.0f);
}

bool RawVolumeRenderer::init() {
	if (!_worldShader.setup()) {
		Log::error("Failed to initialize the color shader");
		return false;
	}

	const int shaderMaterialColorsArraySize = _worldShader.getUniformArraySize("u_materialcolor");
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
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

	_indexBufferIndex = _vertexBuffer.create(nullptr, 0, GL_ELEMENT_ARRAY_BUFFER);
	if (_indexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object for the indices");
		return false;
	}

	video::VertexBuffer::Attribute attributePos;
	attributePos.bufferIndex = _vertexBufferIndex;
	attributePos.index = _worldShader.getLocationPos();
	attributePos.stride = sizeof(voxel::Vertex);
	attributePos.size = _worldShader.getComponentsPos();
	attributePos.type = GL_UNSIGNED_BYTE;
	attributePos.typeIsInt = true;
	attributePos.offset = offsetof(voxel::Vertex, position);
	_vertexBuffer.addAttribute(attributePos);

	video::VertexBuffer::Attribute attributeInfo;
	attributeInfo.bufferIndex = _vertexBufferIndex;
	attributeInfo.index = _worldShader.getLocationInfo();
	attributeInfo.stride = sizeof(voxel::Vertex);
	attributeInfo.size = _worldShader.getComponentsInfo();
	attributeInfo.type = GL_UNSIGNED_BYTE;
	attributeInfo.typeIsInt = true;
	attributeInfo.offset = offsetof(voxel::Vertex, ambientOcclusion);
	_vertexBuffer.addAttribute(attributeInfo);

	_whiteTexture = video::createWhiteTexture("**whitetexture**");

	_mesh = new voxel::Mesh(128, 128, true);

	return true;
}

bool RawVolumeRenderer::onResize(const glm::ivec2& position, const glm::ivec2& dimension) {
	core_trace_scoped(RawVolumeRendererOnResize);
	_sunLight.init(_sunDirection, position, dimension);

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	_depthBuffer.shutdown();
	if (!_depthBuffer.init(_sunLight.dimension(), video::DepthBufferMode::RGBA, maxDepthBuffers)) {
		return false;
	}
	return true;
}

bool RawVolumeRenderer::update(const std::vector<voxel::Vertex>& vertices, const std::vector<voxel::IndexType>& indices) {
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

	struct CustomIsQuadNeeded {
		inline bool operator()(const voxel::Voxel& back, const voxel::Voxel& front, voxel::Voxel& materialToUse, voxel::FaceNames face, int x, int z) const {
			if (back.getMaterial() != voxel::VoxelType::Air
					&& front.getMaterial() == voxel::VoxelType::Air) {
				materialToUse = back;
				return true;
			}
			return false;
		}
	};

	voxel::extractCubicMesh(_rawVolume, _rawVolume->getEnclosingRegion(), _mesh, CustomIsQuadNeeded());
	const voxel::IndexType* meshIndices = _mesh->getRawIndexData();
	const voxel::Vertex* meshVertices = _mesh->getRawVertexData();
	const size_t meshNumberIndices = _mesh->getNoOfIndices();
	if (meshNumberIndices == 0) {
		_vertexBuffer.update(_vertexBufferIndex, nullptr, 0);
		_vertexBuffer.update(_indexBufferIndex, nullptr, 0);
	} else {
		const size_t meshNumberVertices = _mesh->getNoOfVertices();
		if (!_vertexBuffer.update(_vertexBufferIndex, meshVertices, sizeof(voxel::Vertex) * meshNumberVertices)) {
			Log::error("Failed to update the vertex buffer");
			return false;
		}
		if (!_vertexBuffer.update(_indexBufferIndex, meshIndices, sizeof(voxel::IndexType) * meshNumberIndices)) {
			Log::error("Failed to update the index buffer");
			return false;
		}
	}

	return true;
}

void RawVolumeRenderer::render(const video::Camera& camera) {
	core_trace_scoped(RawVolumeRendererRender);
	if (_rawVolume == nullptr) {
		return;
	}

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LEQUAL);
	// Cull triangles whose normal is not towards the camera
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	if (_renderGrid) {
		const voxel::Region& region = _rawVolume->getEnclosingRegion();
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

	const GLuint nIndices = _vertexBuffer.elements(_indexBufferIndex, 1, sizeof(uint32_t));
	if (nIndices == 0) {
		return;
	}

	_sunLight.update(0.0f, camera);

	_whiteTexture->bind(0);

	video::ScopedShader scoped(_worldShader);
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_model", glm::mat4());
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_view", camera.viewMatrix());
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_projection", camera.projectionMatrix());
	shaderSetUniformIf(_worldShader, setUniformVec4v, "u_materialcolor[0]", &materialColors[0], materialColors.size());
	shaderSetUniformIf(_worldShader, setUniformi, "u_texture", 0);
	shaderSetUniformIf(_worldShader, setUniformf, "u_fogrange", 250.0f);
	shaderSetUniformIf(_worldShader, setUniformf, "u_viewdistance", camera.farPlane());
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_light_projection", _sunLight.projectionMatrix());
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_light_view", _sunLight.viewMatrix());
	shaderSetUniformIf(_worldShader, setUniformVec3, "u_lightdir", _sunLight.direction());
	shaderSetUniformIf(_worldShader, setUniformf, "u_depthsize", glm::vec2(_sunLight.dimension()));
	shaderSetUniformIf(_worldShader, setUniformMatrix, "u_light", _sunLight.viewProjectionMatrix(camera));
	shaderSetUniformIf(_worldShader, setUniformVec3, "u_diffuse_color", _diffuseColor);
	shaderSetUniformIf(_worldShader, setUniformVec3, "u_ambient_color", _ambientColor);
	shaderSetUniformIf(_worldShader, setUniformf, "u_debug_color", 1.0);
	shaderSetUniformIf(_worldShader, setUniformf, "u_screensize", glm::vec2(camera.dimension()));
	shaderSetUniformIf(_worldShader, setUniformf, "u_nearplane", camera.nearPlane());
	shaderSetUniformIf(_worldShader, setUniformf, "u_farplane", camera.farPlane());
	shaderSetUniformIf(_worldShader, setUniformVec3, "u_campos", camera.position());
	const bool shadowMap = _worldShader.hasUniform("u_shadowmap1");
	if (shadowMap) {
		const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
		for (int i = 0; i < maxDepthBuffers; ++i) {
			glActiveTexture(GL_TEXTURE1 + i);
			glBindTexture(GL_TEXTURE_2D, _depthBuffer.getTexture(i));
			shaderSetUniformIf(_worldShader, setUniformi, core::string::format("u_shadowmap%i", 1 + i), 1 + i);
		}
	}
	core_assert_always(_vertexBuffer.bind());
	static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
	glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, nullptr);

	if (_renderWireframe && camera.polygonMode() == video::PolygonMode::Solid) {
		video::ScopedPolygonMode polygonMode(video::PolygonMode::WireFrame, glm::vec2(2.0f));
		video::ScopedLineWidth lineWidth(2.0f, true);
		shaderSetUniformIf(_worldShader, setUniformf, "u_debug_color", 0.0);
		glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, nullptr);
	}

	_vertexBuffer.unbind();

	_whiteTexture->unbind();

	if (shadowMap) {
		const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
		for (int i = 0; i < maxDepthBuffers; ++i) {
			glActiveTexture(GL_TEXTURE1 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		glActiveTexture(GL_TEXTURE0);
	}

	GL_checkError();
}

voxel::RawVolume* RawVolumeRenderer::setVolume(voxel::RawVolume* volume) {
	core_trace_scoped(RawVolumeRendererSetVolume);
	voxel::RawVolume* old = _rawVolume;
	_rawVolume = volume;
	if (_rawVolume != nullptr) {
		const voxel::Region& region = _rawVolume->getEnclosingRegion();
		const core::AABB<float> aabb(region.getLowerCorner(), region.getUpperCorner());
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

const voxel::Vertex* RawVolumeRenderer::vertices() const {
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
