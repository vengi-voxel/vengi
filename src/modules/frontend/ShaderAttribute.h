#pragma once

#include "video/VertexBuffer.h"
#include "voxel/polyvox/VoxelVertex.h"
#include "voxel/Constants.h"

namespace frontend {

inline video::VertexBuffer::Attribute getPositionVertexAttribute(uint32_t bufferIndex, uint32_t attributeIndex, int components) {
	static_assert(MAX_TERRAIN_HEIGHT < 256, "Max terrain height exceeds the valid voxel positions");
	video::VertexBuffer::Attribute attrib;
	attrib.bufferIndex = bufferIndex;
	attrib.index = attributeIndex;
	attrib.stride = sizeof(voxel::VoxelVertex);
	attrib.size = components;
	attrib.type = GL_UNSIGNED_BYTE;
	attrib.typeIsInt = true;
	attrib.offset = offsetof(voxel::VoxelVertex, position);
	return attrib;
}

/**
 * @note we are uploading multiple bytes at once here
 */
inline video::VertexBuffer::Attribute getInfoVertexAttribute(uint32_t bufferIndex, uint32_t attributeIndex, int components) {
	static_assert(sizeof(voxel::VoxelVertex::colorIndex) == sizeof(uint8_t), "Voxel color size doesn't match");
	static_assert(sizeof(voxel::VoxelVertex::ambientOcclusion) == sizeof(uint8_t), "AO type size doesn't match");
	static_assert(sizeof(voxel::VoxelVertex::material) == sizeof(uint8_t), "Material type size doesn't match");
	static_assert(offsetof(voxel::VoxelVertex, ambientOcclusion) < offsetof(voxel::VoxelVertex, colorIndex), "Layout change of VoxelVertex without change in upload");
	static_assert(offsetof(voxel::VoxelVertex, ambientOcclusion) < offsetof(voxel::VoxelVertex, material), "Layout change of VoxelVertex without change in upload");
	video::VertexBuffer::Attribute attrib;
	attrib.bufferIndex = bufferIndex;
	attrib.index = attributeIndex;
	attrib.stride = sizeof(voxel::VoxelVertex);
	attrib.size = components;
	attrib.type = GL_UNSIGNED_BYTE;
	attrib.typeIsInt = true;
	attrib.offset = offsetof(voxel::VoxelVertex, ambientOcclusion);
	return attrib;
}

inline video::VertexBuffer::Attribute getOffsetVertexAttribute(uint32_t bufferIndex, uint32_t attributeIndex, int components) {
	video::VertexBuffer::Attribute voxelAttributeOffsets;
	voxelAttributeOffsets.bufferIndex = bufferIndex;
	voxelAttributeOffsets.index = attributeIndex;
	voxelAttributeOffsets.stride = sizeof(glm::vec3);
	voxelAttributeOffsets.size = components;
	voxelAttributeOffsets.type = GL_FLOAT;
	voxelAttributeOffsets.divisor = 1;
	voxelAttributeOffsets.typeIsInt = true;
	voxelAttributeOffsets.offset = offsetof(glm::vec3, x);
	return voxelAttributeOffsets;
}

}
