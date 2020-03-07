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

public:
	video::Buffer _opaqueBuffer;
	int32_t _opaqueIbo = -1;
	int32_t _opaqueVbo = -1;
	video::Buffer _waterBuffer;
	int32_t _waterVbo = -1;

	bool renderOpaqueBuffers();
	bool renderWaterBuffers();

	bool init(shader::WorldShader& worldShader, shader::WaterShader& waterShader);
	void shutdown();
};

}
