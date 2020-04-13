/**
 * @file
 */

#include "WorldBuffers.h"
#include "video/ScopedState.h"
#include "video/Buffer.h"
#include "video/Types.h"
#include "voxel/VoxelVertex.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "video/Renderer.h"
#include "ShaderAttribute.h"

namespace voxelrender {

bool WorldBuffers::renderTerrain() {
	core_trace_gl_scoped(WorldBuffersRenderTerrain);
	const uint32_t numIndices = _buffer.elements(_ibo, 1, sizeof(voxel::IndexType));
	if (numIndices == 0u) {
		return false;
	}
	video::ScopedBuffer scopedBuf(_buffer);
	video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
	return true;
}

bool WorldBuffers::renderWater() {
	core_trace_gl_scoped(WorldBuffersRenderWater);
	video::ScopedBuffer scopedBuf(_waterBuffer);
	const int elements = _waterBuffer.elements(_waterVbo, 2);
	video::drawArrays(video::Primitive::Triangles, elements);
	return true;
}

bool WorldBuffers::initTerrainBuffer(shader::WorldShader& worldShader) {
	_vbo = _buffer.create();
	if (_vbo == -1) {
		Log::error("Failed to create vertex buffer");
		return false;
	}
	_buffer.setMode(_vbo, video::BufferMode::Stream);
	_ibo = _buffer.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (_ibo == -1) {
		Log::error("Failed to create index buffer");
		return false;
	}
	_buffer.setMode(_ibo, video::BufferMode::Stream);

	const int locationPos = worldShader.getLocationPos();
	const video::Attribute& posAttrib = getPositionVertexAttribute(_vbo, locationPos, worldShader.getAttributeComponents(locationPos));
	if (!_buffer.addAttribute(posAttrib)) {
		Log::warn("Failed to add position attribute");
	}

	const int locationInfo = worldShader.getLocationInfo();
	const video::Attribute& infoAttrib = getInfoVertexAttribute(_vbo, locationInfo, worldShader.getAttributeComponents(locationInfo));
	if (!_buffer.addAttribute(infoAttrib)) {
		Log::warn("Failed to add info attribute");
	}

	return true;
}

bool WorldBuffers::initWaterBuffer(shader::WaterShader& waterShader) {
	alignas(16) static constexpr glm::vec2 vecs[] = {
		{ -1.0f, -1.0f},
		{ -1.0f,  1.0f},
		{  1.0f, -1.0f},
		{  1.0f, -1.0f},
		{ -1.0f,  1.0f},
		{  1.0f,  1.0f},

		{ -1.0f, -1.0f},
		{  1.0f, -1.0f},
		{ -1.0f,  1.0f},
		{  1.0f, -1.0f},
		{  1.0f,  1.0f},
		{ -1.0f,  1.0f}
	};
	_waterVbo = _waterBuffer.create(vecs, sizeof(vecs));
	if (_waterVbo == -1) {
		Log::error("Failed to create water vertex buffer");
		return false;
	}

	video::ScopedBuffer scoped(_waterBuffer);
	const int locationPos = waterShader.getLocationPos();
	waterShader.enableVertexAttributeArray(locationPos);
	const video::Attribute& posAttrib = waterShader.getPosAttribute(_waterVbo, &glm::vec2::x);
	if (!_waterBuffer.addAttribute(posAttrib)) {
		Log::warn("Failed to add water position attribute");
	}

	return true;
}

bool WorldBuffers::init(shader::WorldShader& worldShader, shader::WaterShader& waterShader) {
	return initWaterBuffer(waterShader) && initTerrainBuffer(worldShader);
}

void WorldBuffers::update(const voxel::VertexArray& vertices, const voxel::IndexArray& indices) {
	core_trace_gl_scoped(WorldBuffersUpdate);
	_buffer.update(_vbo, vertices);
	_buffer.update(_ibo, indices);
}

void WorldBuffers::shutdown() {
	_buffer.shutdown();
	_waterBuffer.shutdown();
}

}