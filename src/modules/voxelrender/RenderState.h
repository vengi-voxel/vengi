/**
 * @file
 */

#pragma once

#include "core/NonCopyable.h"
#include "video/Buffer.h"
#include "voxel/MeshState.h"

namespace voxelrender {

struct RenderState : public core::NonCopyable {
	RenderState() = default;
	RenderState(RenderState &&other) noexcept
		: _culled(other._culled), _empty(other._empty), _dirtyNormals(other._dirtyNormals),
		  _normalPreviewBufferIndex(other._normalPreviewBufferIndex) {
		for (int i = 0; i < voxel::MeshType_Max; ++i) {
			_vertexBufferIndex[i] = other._vertexBufferIndex[i];
			_normalBufferIndex[i] = other._normalBufferIndex[i];
			_indexBufferIndex[i] = other._indexBufferIndex[i];
			_vertexBuffer[i] = core::move(other._vertexBuffer[i]);
			other._vertexBufferIndex[i] = -1;
			other._normalBufferIndex[i] = -1;
			other._indexBufferIndex[i] = -1;
		}
		other._normalPreviewBufferIndex = -1;
	}
	RenderState &operator=(RenderState &&other) noexcept {
		if (this != &other) {
			_culled = other._culled;
			_empty = other._empty;
			_dirtyNormals = other._dirtyNormals;
			_normalPreviewBufferIndex = other._normalPreviewBufferIndex;
			for (int i = 0; i < voxel::MeshType_Max; ++i) {
				_vertexBufferIndex[i] = other._vertexBufferIndex[i];
				_normalBufferIndex[i] = other._normalBufferIndex[i];
				_indexBufferIndex[i] = other._indexBufferIndex[i];
				_vertexBuffer[i] = core::move(other._vertexBuffer[i]);
				other._vertexBufferIndex[i] = -1;
				other._normalBufferIndex[i] = -1;
				other._indexBufferIndex[i] = -1;
			}
			other._normalPreviewBufferIndex = -1;
		}
		return *this;
	}

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
