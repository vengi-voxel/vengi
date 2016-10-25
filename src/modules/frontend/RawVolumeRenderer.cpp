#include "RawVolumeRenderer.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "frontend/MaterialColor.h"

namespace frontend {

RawVolumeRenderer::RawVolumeRenderer(bool renderAABB) :
		_rawVolume(nullptr), _mesh(nullptr), _worldShader(shader::WorldShader::getInstance()), _renderAABB(renderAABB) {
}

bool RawVolumeRenderer::init(const glm::ivec2& dimension) {
	if (!_worldShader.setup()) {
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

	_indexBufferIndex = _vertexBuffer.create(nullptr, 0, GL_ELEMENT_ARRAY_BUFFER);
	if (_indexBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object for the indices");
		return false;
	}

	const glm::vec3 sunDirection(glm::left.x, glm::down.y, 0.0f);
	_sunLight.init(sunDirection, dimension);

	const int posLoc = _worldShader.enableVertexAttributeArray("a_pos");
	const int components = sizeof(voxel::Vertex::position) / sizeof(decltype(voxel::Vertex::position)::value_type);
	_worldShader.setVertexAttributeInt(posLoc, components, GL_UNSIGNED_BYTE, sizeof(voxel::Vertex), GL_OFFSET_CAST(offsetof(voxel::Vertex, position)));

	const int locationInfo = _worldShader.enableVertexAttributeArray("a_info");
	// we are uploading two bytes at once here
	static_assert(sizeof(voxel::Voxel) == sizeof(uint8_t), "Voxel type doesn't match");
	static_assert(sizeof(voxel::Vertex::ambientOcclusion) == sizeof(uint8_t), "AO type doesn't match");
	_worldShader.setVertexAttributeInt(locationInfo, 2, GL_UNSIGNED_BYTE, sizeof(voxel::Vertex), GL_OFFSET_CAST(offsetof(voxel::Vertex, ambientOcclusion)));
	GL_checkError();

	_mesh = new voxel::Mesh(128, 128, true);

	return true;
}

bool RawVolumeRenderer::update(const std::vector<voxel::Vertex>& vertices, const std::vector<voxel::IndexType>& indices) {
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

void RawVolumeRenderer::render(const video::Camera& camera) {
	if (_renderAABB) {
		_shapeRenderer.render(_aabbMeshIndex, camera);
	}

	const GLuint nIndices = _vertexBuffer.elements(_indexBufferIndex, 1, sizeof(uint32_t));
	if (nIndices == 0) {
		return;
	}

	_sunLight.update(0.0f, camera);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LEQUAL);
	// Cull triangles whose normal is not towards the camera
	glEnable(GL_CULL_FACE);
	glDepthMask(GL_TRUE);

	video::ScopedShader scoped(_worldShader);
	glm::vec3 _diffuseColor = glm::vec3(1.0, 1.0, 1.0);
	const MaterialColorArray& materialColors = getMaterialColors();
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
	shaderSetUniformIf(_worldShader, setUniformf, "u_debug_color", 1.0);
	shaderSetUniformIf(_worldShader, setUniformf, "u_screensize", glm::vec2(camera.dimension()));
	shaderSetUniformIf(_worldShader, setUniformf, "u_nearplane", camera.nearPlane());
	shaderSetUniformIf(_worldShader, setUniformf, "u_farplane", camera.farPlane());
	shaderSetUniformIf(_worldShader, setUniformVec3, "u_campos", camera.position());

	core_assert_always(_vertexBuffer.bind());
	static_assert(sizeof(voxel::IndexType) == sizeof(uint32_t), "Index type doesn't match");
	glDrawElements(GL_TRIANGLES, nIndices, GL_UNSIGNED_INT, nullptr);
	_vertexBuffer.unbind();

	GL_checkError();
}

voxel::RawVolume* RawVolumeRenderer::setVolume(voxel::RawVolume* volume) {
	voxel::RawVolume* old = _rawVolume;
	_rawVolume = volume;
	if (_rawVolume != nullptr) {
		const voxel::Region& region = _rawVolume->getEnclosingRegion();
		const core::AABB<float> aabb(region.getLowerCorner(), region.getUpperCorner());
		_shapeBuilder.clear();
		_shapeBuilder.aabb(aabb);
		if (_aabbMeshIndex == -1) {
			_aabbMeshIndex = _shapeRenderer.createMesh(_shapeBuilder);
		} else {
			_shapeRenderer.update(_aabbMeshIndex, _shapeBuilder);
		}
	} else {
		_shapeBuilder.clear();
	}
	return old;
}

bool RawVolumeRenderer::extract() {
	if (_rawVolume == nullptr) {
		return false;
	}

	if (_mesh == nullptr) {
		return false;
	}

	voxel::extractCubicMesh(_rawVolume, _rawVolume->getEnclosingRegion(), _mesh, voxel::IsQuadNeeded(false));
	const voxel::IndexType* meshIndices = _mesh->getRawIndexData();
	const voxel::Vertex* meshVertices = _mesh->getRawVertexData();
	const size_t meshNumberIndices = _mesh->getNoOfIndices();
	if (meshNumberIndices == 0) {
		_vertexBuffer.update(_vertexBufferIndex, nullptr, 0);
		_vertexBuffer.update(_indexBufferIndex, nullptr, 0);
		return true;
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

voxel::RawVolume* RawVolumeRenderer::shutdown() {
	_vertexBuffer.shutdown();
	_worldShader.shutdown();
	_vertexBufferIndex = -1;
	_indexBufferIndex = -1;
	_aabbMeshIndex = -1;
	if (_mesh != nullptr) {
		delete _mesh;
	}
	_mesh = nullptr;
	voxel::RawVolume* old = _rawVolume;
	_rawVolume = nullptr;
	_shapeRenderer.shutdown();
	_shapeBuilder.shutdown();
	return old;
}

}
