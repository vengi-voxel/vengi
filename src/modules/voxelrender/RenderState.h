/**
 * @file
 */

#pragma once

#include "core/NonCopyable.h"
#include "video/Buffer.h"
#include "voxel/MeshState.h"

namespace voxelrender {

struct RenderState : public core::NonCopyable {
	bool _culled = false;
	bool _empty = false; // this is only updated for non hidden nodes
	bool _dirtyNormals = false;
	int32_t _vertexBufferIndex[voxel::MeshType_Max]{-1, -1};
	int32_t _normalBufferIndex[voxel::MeshType_Max]{-1, -1};
	int32_t _normalPreviewBufferIndex = -1;
	int32_t _indexBufferIndex[voxel::MeshType_Max]{-1, -1};
	video::Buffer _vertexBuffer[voxel::MeshType_Max];

	uint32_t indices(voxel::MeshType type) const;

	bool hasData() const;
};

} // namespace voxelrender
