#include "RawVolumeRenderer.h"
#include "voxel/polyvox/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "frontend/MaterialColor.h"

namespace frontend {

RawVolumeRenderer::RawVolumeRenderer(bool renderAABB) :
		_rawVolume(nullptr), _mesh(nullptr), _colorShader(shader::ColorShader::getInstance()), _renderAABB(renderAABB) {
}

bool RawVolumeRenderer::update(const std::vector<glm::vec4>& positions, const std::vector<uint32_t>& indices, const std::vector<glm::vec3>& colors) {
	if (!_vertexBuffer.update(_vertexBufferIndex, positions)) {
		Log::error("Failed to update the vertex buffer");
		return false;
	}
	if (!_vertexBuffer.update(_indexBufferIndex, indices)) {
		Log::error("Failed to update the index buffer");
		return false;
	}
	if (!_vertexBuffer.update(_colorBufferIndex, colors)) {
		Log::error("Failed to update the color buffer");
		return false;
	}
	_pos = positions;
	_indices = indices;
	_colors = colors;
	return true;
}

void RawVolumeRenderer::render(const video::Camera& camera) {
	if (_renderAABB) {
		_shapeRenderer.render(_aabbMeshIndex, camera);
	}

	if (_pos.empty()) {
		return;
	}
	video::ScopedShader scoped(_colorShader);
	core_assert_always(_colorShader.setView(camera.viewMatrix()));
	core_assert_always(_colorShader.setProjection(camera.projectionMatrix()));

	core_assert_always(_vertexBuffer.bind());
	const GLuint nIndices = _vertexBuffer.elements(_indexBufferIndex, 1, sizeof(uint32_t));
	core_assert(nIndices > 0);
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
	_pos.clear();
	_indices.clear();
	_colors.clear();
	if (meshNumberIndices == 0) {
		return true;
	} else {
		const size_t meshNumberVertices = _mesh->getNoOfVertices();
		_pos.reserve(meshNumberVertices);
		_indices.reserve(meshNumberIndices);
		_colors.reserve(meshNumberVertices);
		const MaterialColorArray& materialColors = getMaterialColors();
		for (size_t i = 0; i < meshNumberVertices; ++i) {
			_pos.emplace_back(meshVertices[i].position, 1.0f);
			_colors.emplace_back(materialColors[meshVertices[i].data.getMaterial()]);
		}
		for (size_t i = 0; i < meshNumberIndices; ++i) {
			_indices.push_back(meshIndices[i]);
		}
		if (!_vertexBuffer.update(_vertexBufferIndex, _pos)) {
			Log::error("Failed to update the vertex buffer");
			return false;
		}
		if (!_vertexBuffer.update(_indexBufferIndex, _indices)) {
			Log::error("Failed to update the index buffer");
			return false;
		}
		if (!_vertexBuffer.update(_colorBufferIndex, _colors)) {
			Log::error("Failed to update the color buffer");
			return false;
		}
	}
	return true;
}

bool RawVolumeRenderer::init() {
	if (!_colorShader.setup()) {
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

	_colorBufferIndex = _vertexBuffer.create();
	if (_colorBufferIndex == -1) {
		Log::error("Could not create the vertex buffer object for the colors");
		return false;
	}

	// configure shader attributes
	core_assert_always(_vertexBuffer.addAttribute(_colorShader.getLocationPos(), _vertexBufferIndex, _colorShader.getComponentsPos()));
	core_assert_always(_vertexBuffer.addAttribute(_colorShader.getLocationColor(), _colorBufferIndex, _colorShader.getComponentsColor()));

	_mesh = new voxel::Mesh(128, 128, true);

	return true;
}

voxel::RawVolume* RawVolumeRenderer::shutdown() {
	_vertexBuffer.shutdown();
	_colorShader.shutdown();
	_vertexBufferIndex = -1;
	_indexBufferIndex = -1;
	_colorBufferIndex = -1;
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
