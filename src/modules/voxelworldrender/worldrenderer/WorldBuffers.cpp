/**
 * @file
 */

#include "WorldBuffers.h"
#include "video/ScopedState.h"
#include "video/Buffer.h"
#include "video/Types.h"
#include "voxel/VoxelVertex.h"
#include "core/Log.h"
#include "video/Trace.h"
#include "video/Renderer.h"

namespace voxelworldrender {

bool WorldBuffers::renderWater() {
	video_trace_scoped(WorldBuffersRenderWater);
	video::ScopedBuffer scopedBuf(_waterBuffer);
	const int elements = _waterBuffer.elements(_waterVbo, 2);
	video::drawArrays(video::Primitive::Triangles, elements);
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

bool WorldBuffers::init(shader::WaterShader& waterShader) {
	return initWaterBuffer(waterShader);
}

void WorldBuffers::shutdown() {
	_waterBuffer.shutdown();
}

}