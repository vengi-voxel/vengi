/**
 * @file
 */

#pragma once

#include "video/Buffer.h"
#include "WorldShader.h"
#include "WaterShader.h"
#include "voxel/Mesh.h"

namespace voxelrender {

class WorldBuffers {
private:
	bool initTerrainBuffer(shader::WorldShader& worldShader);
	bool initWaterBuffer(shader::WaterShader& waterShader);

	video::Buffer _buffer;
	int32_t _ibo = -1;
	int32_t _vbo = -1;
	video::Buffer _waterBuffer;
	int32_t _waterVbo = -1;
public:
	bool renderTerrain();
	bool renderWater();

	void update(const voxel::VertexArray& vertices, const voxel::IndexArray& indices);

	bool init(shader::WorldShader& worldShader, shader::WaterShader& waterShader);
	void shutdown();
};

}
