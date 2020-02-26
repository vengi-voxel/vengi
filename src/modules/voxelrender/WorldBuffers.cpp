/**
 * @file
 */

#include "WorldBuffers.h"
#include "video/ScopedState.h"
#include "video/Buffer.h"
#include "video/Types.h"
#include "voxel/VoxelVertex.h"
#include "core/Log.h"
#include "video/Renderer.h"
#include "ShaderAttribute.h"

namespace voxelrender {

bool WorldBuffers::renderOpaqueBuffers() {
	const uint32_t numIndices = _opaqueBuffer.elements(_opaqueIbo, 1, sizeof(voxel::IndexType));
	if (numIndices == 0u) {
		return false;
	}
	video::ScopedBuffer scopedBuf(_opaqueBuffer);
	video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
	return true;
}

bool WorldBuffers::renderWaterBuffers() {
	const uint32_t numIndices = _waterBuffer.elements(_waterIbo, 1, sizeof(voxel::IndexType));
	if (numIndices == 0u) {
		return false;
	}
	video::ScopedState cullFace(video::State::CullFace, false);
	video::ScopedBuffer scopedBuf(_waterBuffer);
	video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
	return true;
}

bool WorldBuffers::initOpaqueBuffer(shader::WorldShader& worldShader) {
	_opaqueVbo = _opaqueBuffer.create();
	if (_opaqueVbo == -1) {
		Log::error("Failed to create vertex buffer");
		return false;
	}
	_opaqueBuffer.setMode(_opaqueVbo, video::BufferMode::Stream);
	_opaqueIbo = _opaqueBuffer.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (_opaqueIbo == -1) {
		Log::error("Failed to create index buffer");
		return false;
	}
	_opaqueBuffer.setMode(_opaqueIbo, video::BufferMode::Stream);

	const int locationPos = worldShader.getLocationPos();
	const video::Attribute& posAttrib = getPositionVertexAttribute(_opaqueVbo, locationPos, worldShader.getAttributeComponents(locationPos));
	if (!_opaqueBuffer.addAttribute(posAttrib)) {
		Log::warn("Failed to add position attribute");
	}

	const int locationInfo = worldShader.getLocationInfo();
	const video::Attribute& infoAttrib = getInfoVertexAttribute(_opaqueVbo, locationInfo, worldShader.getAttributeComponents(locationInfo));
	if (!_opaqueBuffer.addAttribute(infoAttrib)) {
		Log::warn("Failed to add info attribute");
	}

	return true;
}

bool WorldBuffers::initWaterBuffer(shader::WaterShader& waterShader) {
	_waterVbo = _waterBuffer.create();
	if (_waterVbo == -1) {
		Log::error("Failed to create water vertex buffer");
		return false;
	}
	_waterBuffer.setMode(_waterVbo, video::BufferMode::Stream);
	_waterIbo = _waterBuffer.create(nullptr, 0, video::BufferType::IndexBuffer);
	if (_waterIbo == -1) {
		Log::error("Failed to create water index buffer");
		return false;
	}
	_waterBuffer.setMode(_waterIbo, video::BufferMode::Stream);

	video::ScopedBuffer scoped(_waterBuffer);
	const int locationPos = waterShader.getLocationPos();
	waterShader.enableVertexAttributeArray(locationPos);
	const video::Attribute& posAttrib = getPositionVertexAttribute(_waterVbo, locationPos, waterShader.getAttributeComponents(locationPos));
	if (!_waterBuffer.addAttribute(posAttrib)) {
		Log::warn("Failed to add water position attribute");
	}

	const int locationInfo = waterShader.getLocationInfo();
	waterShader.enableVertexAttributeArray(locationInfo);
	const video::Attribute& infoAttrib = getInfoVertexAttribute(_waterVbo, locationInfo, waterShader.getAttributeComponents(locationInfo));
	if (!_waterBuffer.addAttribute(infoAttrib)) {
		Log::warn("Failed to add water info attribute");
	}

	return true;
}

bool WorldBuffers::init(shader::WorldShader& worldShader, shader::WaterShader& waterShader) {
	return initWaterBuffer(waterShader) && initOpaqueBuffer(worldShader);
}

void WorldBuffers::shutdown() {
	_opaqueBuffer.shutdown();
	_waterBuffer.shutdown();
}

}