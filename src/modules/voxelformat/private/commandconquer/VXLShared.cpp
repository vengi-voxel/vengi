/**
 * @file
 */

#include "VXLShared.h"
#include "core/Log.h"
#include "scenegraph/CoordinateSystemUtil.h"

namespace voxelformat {

namespace vxl {

void VXLMatrix::fromVengi(const glm::mat4 &vengiMatrix) {
	matrix = scenegraph::convertCoordinateSystem(scenegraph::CoordinateSystem::Vengi, scenegraph::CoordinateSystem::VXL,
												 vengiMatrix);
}

glm::mat4 VXLMatrix::toVengi() const {
	return scenegraph::convertCoordinateSystem(scenegraph::CoordinateSystem::VXL, scenegraph::CoordinateSystem::Vengi,
											   matrix);
}

int vxl::VXLModel::findLayerByName(const core::String &name) const {
	for (uint32_t i = 0; i < header.layerCount; ++i) {
		if (name == layerHeaders[i].name) {
			return (int)i;
		}
	}
	return -1;
}

void convertWrite(VXLMatrix &vxlMatrix, const glm::mat4 &vengiMatrix, const glm::vec3 &mins, bool hva,
				  const voxel::Region &region) {
	Log::debug("ConvertWrite: Initial translation: %f %f %f", vengiMatrix[3].x, vengiMatrix[3].y, vengiMatrix[3].z);
	Log::debug("ConvertWrite: Local translate: %f %f %f", mins.x, mins.y, mins.z);
	glm::mat4 originalMatrix = vengiMatrix;

	if (hva) {
		// the hva matrices have to be scaled
		// Calculate the ratio between screen units and voxels in all dimensions
		const glm::ivec3 &size = region.getDimensionsInVoxels();
		const glm::vec3 maxs = mins + glm::vec3(size);
		const glm::vec3 sectionScale{(maxs.x - mins.x) / (float)size.x, (maxs.y - mins.y) / (float)size.y,
									 (maxs.z - mins.z) / (float)size.z};
		Log::debug("ConvertWrite: Section scale: %f %f %f", sectionScale.x, sectionScale.y, sectionScale.z);
		// swap y and z here
		originalMatrix[3].x /= (vxl::Scale * sectionScale.x);
		originalMatrix[3].y /= (vxl::Scale * sectionScale.y);
		originalMatrix[3].z /= (vxl::Scale * sectionScale.z);
		Log::debug("ConvertWrite: Final translation: %f %f %f", originalMatrix[3].x, originalMatrix[3].y,
				   originalMatrix[3].z);
	}

	vxlMatrix.fromVengi(originalMatrix);

	Log::debug("ConvertWrite: vxl translation: %f %f %f", vxlMatrix.matrix[3].x, vxlMatrix.matrix[3].y,
			   vxlMatrix.matrix[3].z);
}

} // namespace vxl
} // namespace voxelformat
