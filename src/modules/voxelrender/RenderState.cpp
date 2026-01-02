/**
 * @file
 */

#include "RenderState.h"
#include "voxel/VoxelVertex.h"

namespace voxelrender {

uint32_t RenderState::indices(voxel::MeshType type) const {
	return _vertexBuffer[type].elements(_indexBufferIndex[type], 1, sizeof(voxel::IndexType));
}

bool RenderState::hasData() const {
	return indices(voxel::MeshType_Opaque) > 0 || indices(voxel::MeshType_Transparency) > 0;
}

} // namespace voxelrender
