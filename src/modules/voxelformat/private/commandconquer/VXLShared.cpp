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

glm::mat4 convertVXLRead(const VXLMatrix &matrix, const vxl::VXLLayerInfo &footer) {
	// The VXL base pose matrix is not scaled by the global footer.scale.
	// The per-section scale is applied by the scene graph transform.
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

// HVA matrices contain transformations (rotation, translation) for animated sections.
// This function converts the HVA transformation into the vengi engine's coordinate system and applies the necessary scaling.
//
// Westwood VXL/HVA Coordinate System: Z-up, right-handed (X=right, Y=forward, Z=up)
// Vengi/OpenGL Coordinate System:    Y-up, right-handed (X=right, Y=up, Z=backward)
//
// The HVA transformation is applied as follows:
// 1. The HVA matrix is converted from VXL to vengi coordinate system.
// 2. The translation part of the resulting matrix is scaled by the global `footer.scale` (typically 1.0/12.0).
// 3. The per-section scale, calculated from the section's bounding box (mins/maxs), is NOT applied here. It is handled by the scenegraph node's transform.
//
// See https://github.com/vengi-voxel/vengi/issues/537 and https://github.com/vengi-voxel/vengi/issues/636
glm::mat4 convertHVARead(const VXLMatrix &matrix, const vxl::VXLLayerInfo &footer) {
	glm::mat4 vengiMatrix = matrix.toVengi();
	glm::vec4 &translation = vengiMatrix[3];

	// Only the HVA translation is scaled by the global footer.scale.
	// The rotation part of the matrix is preserved.
	translation.x *= footer.scale;
	translation.y *= footer.scale;
	translation.z *= footer.scale;
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
//
// Understanding VXL pivot and scale (from issue #537):
//
// 1. The 1/12th scale (vxl::Scale) is a constant conversion factor to bring voxels to pixel size on screen
// 2. HVA movement is based on "leptons" - a cell-based measurement that doesn't scale with voxel shrinkage
// 3. Each section can have its own scale factor (calculated from bounds), they cannot all be assumed the same
// 4. Voxel offset (mins/maxs) scales with voxel shrinkage
// 5. HVA positioning doesn't scale because it's lepton-based
// 6. The rotation point is always at voxel coordinate (0,0,0) regardless of physical offset
// 7. A voxel can be off-center physically but rotation happens at 0,0,0
//
// For example, helicopter rotors:
// - Rotor can be offset laterally/vertically using voxel offset (mins/maxs in VXL)
// - Can be offset further using HVA position
// - These two offsets can be in opposite directions
// - Voxel offset scales with section scale, HVA offset does NOT
//
// y and z flipped to bring it into vengi space
glm::vec3 VXLLayerInfo::pivot() const {
	// mins represents the offset of voxel data from origin in VXL coordinate system
	// Default mins = -size/2 (centers the voxel around origin)
	// mins can be adjusted to offset the voxel
	//
	// The pivot is the normalized position within the bounding box where the origin (0,0,0) is located
	// Formula: pivot = -mins / (maxs - mins)
	// This gives values typically around 0.5 (centered) but can vary if mins/maxs is adjusted
	//
	// Must account for coordinate system conversion
	// VXL coords (x,y,z) -> vengi coords (x,z,y) means:
	// - VXL mins.x -> vengi pivot.x (divided by (maxs.x - mins.x))
	// - VXL mins.y -> vengi pivot.z (divided by (maxs.y - mins.y))
	// - VXL mins.z -> vengi pivot.y (divided by (maxs.z - mins.z))
	//
	const float spanX = maxs.x - mins.x;
	const float spanY = maxs.y - mins.y;
	const float spanZ = maxs.z - mins.z;
	glm::vec3 pivot{-mins.x / spanX, -mins.z / spanZ, -mins.y / spanY};
	Log::debug("Pivot: %f:%f:%f", pivot.x, pivot.y, pivot.z);
	return pivot;
}

} // namespace vxl
} // namespace voxelformat
