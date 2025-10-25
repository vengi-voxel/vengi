/**
 * @file
 */

#include "VXLShared.h"
#include "core/Log.h"
#include "math/CoordinateSystemUtil.h"
#include <glm/ext/matrix_transform.hpp>

namespace voxelformat {

namespace vxl {

void VXLMatrix::fromVengi(const glm::mat4 &vengiMatrix) {
	matrix = math::convertCoordinateSystem(math::CoordinateSystem::Vengi, math::CoordinateSystem::VXL, vengiMatrix);
}

glm::mat4 VXLMatrix::toVengi() const {
	return math::convertCoordinateSystem(math::CoordinateSystem::VXL, math::CoordinateSystem::Vengi, matrix);
}

int vxl::VXLModel::findLayerByName(const core::String &name) const {
	for (uint32_t i = 0; i < header.layerCount; ++i) {
		if (name == layerHeaders[i].name) {
			return (int)i;
		}
	}
	return -1;
}

glm::mat4 convertVXLRead(const VXLMatrix &matrix) {
	// TODO: VOXELFORMAT: scaling needed?
	return matrix.toVengi();
}

vxl::VXLMatrix convertVXLWrite(const glm::mat4 &vengiMatrix) {
	vxl::VXLMatrix vxlMatrix;
	glm::mat4 originalMatrix = vengiMatrix;
	vxlMatrix.fromVengi(originalMatrix);
	Log::debug("ConvertWrite: vxl translation: %f %f %f", vxlMatrix.matrix[3].x, vxlMatrix.matrix[3].y,
			   vxlMatrix.matrix[3].z);
	return vxlMatrix;
}

VXLMatrix convertHVAWrite(const glm::mat4 &vengiMatrix) {
	VXLMatrix vxlMatrix;
	glm::mat4 originalMatrix = vengiMatrix;
	glm::vec4 &translation = originalMatrix[3];
	translation.x /= vxl::Scale;
	translation.y /= vxl::Scale;
	translation.z /= vxl::Scale;
	vxlMatrix.fromVengi(originalMatrix);
	Log::debug("ConvertWrite: vxl translation: %f %f %f", vxlMatrix.matrix[3].x, vxlMatrix.matrix[3].y,
			   vxlMatrix.matrix[3].z);
	return vxlMatrix;
}

glm::mat4 convertHVARead(const VXLMatrix &matrix, const vxl::VXLLayerInfo &footer) {
	glm::mat4 vengiMatrix = matrix.toVengi();
	glm::vec4 &translation = vengiMatrix[3];
	// the hva matrices have to be scaled
	const glm::vec3 sectionScale = footer.calcScale();
	vengiMatrix = glm::scale(vengiMatrix, sectionScale);
	translation.x *= footer.scale * sectionScale.x;
	translation.y *= footer.scale * sectionScale.y;
	translation.z *= footer.scale * sectionScale.z;
	Log::debug("ConvertRead: vxl translation: %f %f %f", translation.x, translation.y, translation.z);
	return vengiMatrix;
}

// y and z flipped to bring it into vengi space
glm::vec3 VXLLayerInfo::calcScale() const {
	const glm::vec3 s{(maxs[0] - mins[0]) / (float)xsize, (maxs[2] - mins[2]) / (float)zsize, (maxs[1] - mins[1]) / (float)ysize};
	Log::debug("Scale: %f:%f:%f", s.x, s.y, s.z);
	return s;
}

// y and z flipped to bring it into vengi space
glm::vec3 VXLLayerInfo::offset() const {
	const glm::vec3 offset(mins[0], mins[2], mins[1]);
	Log::debug("Offset: %f:%f:%f", offset.x, offset.y, offset.z);
	return offset;
}

// TODO: VOXELFORMAT: pivot handling is broken (https://github.com/vengi-voxel/vengi/issues/537)
// TODO: VOXELFORMAT: https://github.com/vengi-voxel/vengi/issues/636
// y and z flipped to bring it into vengi space
glm::vec3 VXLLayerInfo::pivot() const {
	// mins represents the offset of voxel data from origin
	// Default mins = -size/2 (centers the voxel around origin)
	// mins can be adjusted to offset the voxel
	//
	// The pivot is the normalized position within the bounding box where the origin (0,0,0) is located
	// Formula: pivot = -mins / size
	// This gives values typically around 0.5 (centered) but can vary if mins is adjusted
	//
	glm::vec3 pivot{-mins.x / (float)xsize, -mins.z / (float)zsize, -mins.y / (float)ysize};
	Log::debug("Pivot: %f:%f:%f", pivot.x, pivot.y, pivot.z);
	return pivot;
}

} // namespace vxl
} // namespace voxelformat
