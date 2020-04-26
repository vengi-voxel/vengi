/**
 * @file
 */

#include "MeshRenderer.h"
#include "core/Log.h"
#include "ShaderAttribute.h"
#include "voxel/MaterialColor.h"
#include "VoxelShaderConstants.h"
#include "core/Trace.h"
#include "video/ScopedState.h"
#include "frontend/Colors.h"
#include "video/Camera.h"
#include "voxel/Mesh.h"

namespace voxelrender {

MeshRenderer::MeshRenderer() :
	_voxelShader(shader::VoxelShader::getInstance()),
	_shadowMapShader(shader::ShadowmapShader::getInstance()) {
}

bool MeshRenderer::init() {
	if (!_voxelShader.setup()) {
		Log::error("Failed to setup voxel shader");
		return false;
	}
	if (!_shadowMapShader.setup()) {
		Log::error("Failed to setup shadowmap shader");
		return false;
	}

	for (auto& mesh : _meshes) {
		mesh.model = glm::mat4(1.0f);
		mesh.vbo = mesh.buffer.create();
		if (mesh.vbo == -1) {
			Log::error("Could not create the vertex buffer object");
			return false;
		}
		mesh.ibo = mesh.buffer.create(nullptr, 0, video::BufferType::IndexBuffer);
		if (mesh.ibo == -1) {
			Log::error("Could not create the vertex buffer object for the indices");
			return false;
		}
		const video::Attribute& attributePos = getPositionVertexAttribute(
				mesh.vbo, _voxelShader.getLocationPos(),
				_voxelShader.getComponentsPos());
		mesh.buffer.addAttribute(attributePos);

		const video::Attribute& attributeInfo = getInfoVertexAttribute(
				mesh.vbo, _voxelShader.getLocationInfo(),
				_voxelShader.getComponentsInfo());
		mesh.buffer.addAttribute(attributeInfo);
	}

	const int shaderMaterialColorsArraySize = lengthof(shader::VoxelData::MaterialblockData::materialcolor);
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	render::ShadowParameters shadowParams;
	shadowParams.maxDepthBuffers = shader::VoxelShaderConstants::getMaxDepthBuffers();
	if (!_shadow.init(shadowParams)) {
		return false;
	}

	shader::VoxelData::MaterialblockData materialBlock;
	memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	_materialBlock.create(materialBlock);

	return true;
}

void MeshRenderer::shutdown() {
	_voxelShader.shutdown();
	_shadowMapShader.shutdown();
	_shadow.shutdown();
	for (auto& mesh : _meshes) {
		mesh.buffer.shutdown();
		mesh.vbo = mesh.ibo = -1;
		mesh.model = glm::mat4(1.0f);
	}
}

bool MeshRenderer::isEmpty() const {
	uint32_t numIndices = 0u;
	for (const auto& mesh : _meshes) {
		numIndices += mesh.numIndices();
		if (numIndices > 0) {
			break;
		}
	}
	return numIndices == 0u;
}

void MeshRenderer::prepareState() {
	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);
}

void MeshRenderer::renderShadows(const video::Camera& camera) {
	_shadow.update(camera, true);
	_shadowMapShader.activate();
	_shadow.render([this] (int i, const glm::mat4& lightViewProjection) {
		_shadowMapShader.setLightviewprojection(lightViewProjection);
		for (const auto& mesh : _meshes) {
			const uint32_t nIndices = mesh.numIndices();
			if (nIndices == 0) {
				continue;
			}
			video::ScopedBuffer scopedBuf(mesh.buffer);
			_shadowMapShader.setModel(mesh.model);
			video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
		}
		return true;
	});
	_shadowMapShader.deactivate();
}

void MeshRenderer::prepareShader(const video::Camera& camera) {
	if (_voxelShader.isDirty()) {
		_voxelShader.setMaterialblock(_materialBlock);
		_voxelShader.setModel(glm::mat4(1.0f));
		_voxelShader.setShadowmap(video::TextureUnit::One);
		_voxelShader.setDiffuseColor(frontend::diffuseColor);
		_voxelShader.setAmbientColor(frontend::ambientColor);
		_voxelShader.markClean();
	}
	_voxelShader.setViewprojection(camera.viewProjectionMatrix());
	_voxelShader.setDepthsize(glm::vec2(_shadow.dimension()));
	_voxelShader.setCascades(_shadow.cascades());
	_voxelShader.setDistances(_shadow.distances());
	_voxelShader.setLightdir(_shadow.sunDirection());
	_shadow.bind(video::TextureUnit::One);
}

void MeshRenderer::render(int idx, const video::Camera& camera) {
	core_trace_scoped(MeshRendererRender);
	if (idx < 0 || idx >= (int)_meshes.size()) {
		Log::trace("Invalid index given in MeshRenderer::setMesh(): %i", idx);
		return;
	}
	MeshInternal& mesh = _meshes[idx];
	const uint32_t nIndices = mesh.numIndices();
	if (nIndices == 0) {
		return;
	}
	prepareState();
	renderShadows(camera);

	video::ScopedShader scoped(_voxelShader);
	prepareShader(camera);
	_voxelShader.setModel(mesh.model);

	video::ScopedBuffer scopedBuf(mesh.buffer);
	video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
}

void MeshRenderer::renderAll(const video::Camera &camera) {
	core_trace_scoped(MeshRendererRender);
	if (isEmpty()) {
		return;
	}
	prepareState();
	renderShadows(camera);

	video::ScopedShader scoped(_voxelShader);
	prepareShader(camera);

	for (const auto& mesh : _meshes) {
		const uint32_t nIndices = mesh.numIndices();
		if (nIndices == 0) {
			continue;
		}
		video::ScopedBuffer scopedBuf(mesh.buffer);
		_voxelShader.setModel(mesh.model);
		video::drawElements<voxel::IndexType>(video::Primitive::Triangles, nIndices);
	}
}

bool MeshRenderer::update(int idx, const voxel::VoxelVertex* vertices, size_t numVertices, const voxel::IndexType* indices, size_t numIndices) {
	core_trace_scoped(MeshRendererUpdate);

	MeshInternal& entry = _meshes[idx];
	if (numIndices == 0) {
		entry.buffer.update(entry.vbo, nullptr, 0);
		entry.buffer.update(entry.ibo, nullptr, 0);
		return true;
	}

	if (!entry.buffer.update(entry.vbo, vertices, numVertices * sizeof(voxel::VoxelVertex))) {
		Log::error("Failed to update the vertex buffer");
		return false;
	}
	if (!entry.buffer.update(entry.ibo, indices, numIndices * sizeof(voxel::IndexType))) {
		Log::error("Failed to update the index buffer");
		return false;
	}
	return true;
}

int MeshRenderer::addMesh(const voxel::Mesh* mesh, const glm::mat4& model) {
	const int currentIndex = _meshIndex % maxMeshes();
	if (!setMesh(currentIndex, mesh, model)) {
		return -1;
	}
	++_meshIndex;
	return currentIndex;
}

bool MeshRenderer::setMesh(int idx, const voxel::Mesh *mesh, const glm::mat4 &model) {
	if (idx < 0 || idx >= (int)_meshes.size()) {
		Log::trace("Invalid index given in MeshRenderer::setMesh(): %i", idx);
		return false;
	}

	MeshInternal& entry = _meshes[idx];
	entry.mesh = mesh;
	entry.model = model;

	if (entry.mesh != nullptr) {
		const voxel::VoxelVertex* vertices = entry.mesh->getRawVertexData();
		const size_t numVertices = entry.mesh->getNoOfVertices();
		const voxel::IndexType* indices = entry.mesh->getRawIndexData();
		const size_t numIndices = entry.mesh->getNoOfIndices();
		return update(idx, vertices, numVertices, indices, numIndices);
	}
	return update(idx, nullptr, 0, nullptr, 0);
}

bool MeshRenderer::setModelMatrix(int idx, const glm::mat4& model) {
	if (idx < 0 || idx >= (int)_meshes.size()) {
		Log::trace("Invalid index given in MeshRenderer::setMesh(): %i", idx);
		return false;
	}
	MeshInternal& entry = _meshes[idx];
	entry.model = model;
	return true;
}

}