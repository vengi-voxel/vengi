/**
 * @file
 */

#pragma once

#include "video/Buffer.h"
#include "WorldShader.h"
#include "WaterShader.h"

namespace voxelrender {

class WorldBuffers {
private:
	bool initOpaqueBuffer(shader::WorldShader& worldShader);
	bool initWaterBuffer(shader::WaterShader& waterShader);

	video::Buffer _waterBuffer;
	int32_t _waterVbo = -1;
public:
	video::Buffer _buffer;
	int32_t _ibo = -1;
	int32_t _vbo = -1;

	bool renderOpaqueBuffers();
	bool renderWaterBuffers();

	bool init(shader::WorldShader& worldShader, shader::WaterShader& waterShader);
	void shutdown();
};

}
