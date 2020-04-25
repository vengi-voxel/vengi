/**
 * @file
 */

#pragma once

#include "video/Buffer.h"
#include "WaterShader.h"
#include "voxel/Mesh.h"

namespace voxelrender {

class WorldBuffers {
private:
	bool initWaterBuffer(shader::WaterShader& waterShader);

	video::Buffer _waterBuffer;
	int32_t _waterVbo = -1;
public:
	bool renderWater();

	bool init(shader::WaterShader& waterShader);
	void shutdown();
};

}
