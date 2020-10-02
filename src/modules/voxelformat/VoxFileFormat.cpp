/**
 * @file
 */

#include "VoxFileFormat.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/MaterialColor.h"
#include "VolumeFormat.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Color.h"
#include "voxel/Mesh.h"
#include "voxelformat/VoxelVolumes.h"
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

MeshExporter::MeshExt::MeshExt(voxel::Mesh *_mesh, const core::String &_name) :
		mesh(_mesh), name(_name) {
}

bool MeshExporter::saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) {
	const bool mergeQuads = core::Var::get("voxformat_mergequads", "true", core::CV_NOPERSIST)->boolVal();
	const bool reuseVertices = core::Var::get("voxformat_reusevertices", "true", core::CV_NOPERSIST)->boolVal();
	const bool ambientOcclusion = core::Var::get("voxformat_ambientocclusion", "false", core::CV_NOPERSIST)->boolVal();
	const float scale = core::Var::get("voxformat_scale", "1.0", core::CV_NOPERSIST)->floatVal();
	const bool quads = core::Var::get("voxformat_quads", "true", core::CV_NOPERSIST)->boolVal();
	const bool withColor = core::Var::get("voxformat_withcolor", "true", core::CV_NOPERSIST)->boolVal();
	const bool withTexCoords = core::Var::get("voxformat_withtexcoords", "true", core::CV_NOPERSIST)->boolVal();

	Meshes meshes;
	for (const VoxelVolume& v : volumes) {
		voxel::Mesh *mesh = new voxel::Mesh();
		voxel::Region region = v.volume->region();
		region.shiftUpperCorner(1, 1, 1);
		voxel::extractCubicMesh(v.volume, region, mesh, voxel::IsQuadNeeded(), glm::ivec3(0), mergeQuads, reuseVertices, ambientOcclusion);
		meshes.emplace_back(mesh, v.name);
	}
	Log::debug("Save meshes");
	const bool state = saveMeshes(meshes, file, scale, quads, withColor, withTexCoords);
	for (MeshExt& meshext : meshes) {
		delete meshext.mesh;
	}
	return state;
}

}
