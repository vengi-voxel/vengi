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

void convertWrite(VXLMatrix &vxlMatrix, const glm::mat4 &vengiMatrix, bool hva) {
	glm::mat4 originalMatrix = vengiMatrix;
	if (hva) {
		glm::vec4 &translation = originalMatrix[3];
		translation.x /= vxl::Scale;
		translation.y /= vxl::Scale;
		translation.z /= vxl::Scale;
	}

	vxlMatrix.fromVengi(originalMatrix);

	Log::debug("ConvertWrite: vxl translation: %f %f %f", vxlMatrix.matrix[3].x, vxlMatrix.matrix[3].y,
			   vxlMatrix.matrix[3].z);
}

} // namespace vxl
} // namespace voxelformat
