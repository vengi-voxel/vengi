/**
 * @file
 */

#include "VoxFileFormat.h"
#include "core/Var.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/MaterialColor.h"
#include "VolumeFormat.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Color.h"
#include "voxel/Mesh.h"
#include <limits>

namespace voxel {

const glm::vec4& VoxFileFormat::getColor(const Voxel& voxel) const {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	return materialColors[voxel.getColor()];
}

uint8_t VoxFileFormat::convertPaletteIndex(uint32_t paletteIndex) const {
	if (paletteIndex >= _paletteSize) {
		if (_paletteSize > 0) {
			return paletteIndex % _paletteSize;
		}
		return paletteIndex % _palette.size();
	}
	return _palette[paletteIndex];
}

glm::vec4 VoxFileFormat::findClosestMatch(const glm::vec4& color) const {
	const int index = findClosestIndex(color);
	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	return materialColors[index];
}

uint8_t VoxFileFormat::findClosestIndex(const glm::vec4& color) const {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	//materialColors.erase(materialColors.begin());
	return core::Color::getClosestMatch(color, materialColors);
}

RawVolume* VoxFileFormat::merge(const VoxelVolumes& volumes) const {
	return volumes.merge();
}

RawVolume* VoxFileFormat::load(const io::FilePtr& file) {
	VoxelVolumes volumes;
	if (!loadGroups(file, volumes)) {
		voxelformat::clearVolumes(volumes);
		return nullptr;
	}
	RawVolume* mergedVolume = merge(volumes);
	voxelformat::clearVolumes(volumes);
	return mergedVolume;
}

bool VoxFileFormat::save(const RawVolume* volume, const io::FilePtr& file) {
	VoxelVolumes volumes;
	volumes.push_back(VoxelVolume(const_cast<RawVolume*>(volume)));
	return saveGroups(volumes, file);
}

bool MeshExporter::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	voxel::RawVolume *volume = volumes.merge();
	if (volume == nullptr) {
		Log::error("Could not merge volumes");
		return false;
	}

	bool mergeQuads = core::Var::get("voxformat_mergequads", "true", core::CV_NOPERSIST)->boolVal();
	bool reuseVertices = core::Var::get("voxformat_reusevertices", "true", core::CV_NOPERSIST)->boolVal();

	voxel::Mesh mesh;
	voxel::Region region = volume->region();
	region.shiftUpperCorner(1, 1, 1);
	voxel::extractCubicMesh(volume, region, &mesh, voxel::IsQuadNeeded(), glm::ivec3(0), mergeQuads, reuseVertices);
	delete volume;
	return saveMesh(mesh, file);
}

}
