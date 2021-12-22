/**
 * @file
 */

#include "Format.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "voxel/CubicSurfaceExtractor.h"
#include "voxel/IsQuadNeeded.h"
#include "voxel/MaterialColor.h"
#include "VolumeFormat.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Color.h"
#include "math/Math.h"
#include "voxel/Mesh.h"
#include "voxelformat/VoxelVolumes.h"
#include <limits>

namespace voxel {

const glm::vec4& Format::getColor(const Voxel& voxel) const {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	return materialColors[voxel.getColor()];
}

uint8_t Format::convertPaletteIndex(uint32_t paletteIndex) const {
	if (paletteIndex >= _paletteSize) {
		if (_paletteSize > 0) {
			return paletteIndex % _paletteSize;
		}
		return paletteIndex % _palette.size();
	}
	return _palette[paletteIndex];
}

glm::vec4 Format::findClosestMatch(const glm::vec4& color) const {
	const int index = findClosestIndex(color);
	voxel::MaterialColorArray materialColors = voxel::getMaterialColors();
	return materialColors[index];
}

uint8_t Format::findClosestIndex(const glm::vec4& color) const {
	const voxel::MaterialColorArray& materialColors = voxel::getMaterialColors();
	//materialColors.erase(materialColors.begin());
	return core::Color::getClosestMatch(color, materialColors);
}

void Format::splitVolumes(const VoxelVolumes& srcVolumes, VoxelVolumes& destVolumes, const glm::ivec3 &maxSize) {
	destVolumes.reserve(srcVolumes.size());
	for (VoxelVolume &v : srcVolumes) {
		if (v.volume == nullptr) {
			continue;
		}
		const voxel::Region& region = v.volume->region();
		if (glm::all(glm::lessThan(region.getDimensionsInVoxels(), maxSize))) {
			destVolumes.push_back(core::move(v));
			continue;
		}
		split(destVolumes, v, maxSize);
		delete v.volume;
		v.volume = nullptr;
	}
}

bool Format::isEmptyBlock(const voxel::RawVolume *v, const glm::ivec3 &maxSize, int x, int y, int z) const {
	const voxel::Region region(x, y, z, x + maxSize.x - 1, y + maxSize.y - 1, z + maxSize.z - 1);
	voxel::RawVolume::Sampler sampler(v);
	for (int32_t x = region.getLowerX(); x <= region.getUpperX(); x += 1) {
		for (int32_t y = region.getLowerY(); y <= region.getUpperY(); y += 1) {
			sampler.setPosition(x, y, region.getLowerZ());
			for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); z += 1) {
				if (voxel::isBlocked(sampler.voxel().getMaterial())) {
					return false;
				}
				sampler.movePositiveZ();
			}
		}
	}
	return true;
}

void Format::calcMinsMaxs(const voxel::Region& region, const glm::ivec3 &maxSize, glm::ivec3 &mins, glm::ivec3 &maxs) const {
	const glm::ivec3 &lower = region.getLowerCorner();
	mins[0] = lower[0] & ~(maxSize.x - 1);
	mins[1] = lower[1] & ~(maxSize.y - 1);
	mins[2] = lower[2] & ~(maxSize.z - 1);

	const glm::ivec3 &upper = region.getUpperCorner();
	maxs[0] = (upper[0] & ~(maxSize.x - 1)) + maxSize.x - 1;
	maxs[1] = (upper[1] & ~(maxSize.y - 1)) + maxSize.y - 1;
	maxs[2] = (upper[2] & ~(maxSize.z - 1)) + maxSize.z - 1;

	Log::debug("%s", region.toString().c_str());
	Log::debug("mins(%i:%i:%i)", mins.x, mins.y, mins.z);
	Log::debug("maxs(%i:%i:%i)", maxs.x, maxs.y, maxs.z);
}

void Format::split(VoxelVolumes& destVolumes, const VoxelVolume &v, const glm::ivec3& maxSize) {
	voxel::RawVolume *copy = new voxel::RawVolume(v.volume);
	// TODO: perform the split
	destVolumes.push_back(VoxelVolume{copy, v.name, v.visible});
}

RawVolume* Format::merge(const VoxelVolumes& volumes) const {
	return volumes.merge();
}

RawVolume* Format::load(const core::String &filename, io::SeekableReadStream& file) {
	VoxelVolumes volumes;
	if (!loadGroups(filename, file, volumes)) {
		voxelformat::clearVolumes(volumes);
		return nullptr;
	}
	RawVolume* mergedVolume = merge(volumes);
	voxelformat::clearVolumes(volumes);
	return mergedVolume;
}

size_t Format::loadPalette(const core::String &filename, io::SeekableReadStream& file, core::Array<uint32_t, 256> &palette) {
	VoxelVolumes volumes;
	loadGroups(filename, file, volumes);
	voxelformat::clearVolumes(volumes);
	palette = _colors;
	return _colorsSize;
}

image::ImagePtr Format::loadScreenshot(const core::String &filename, io::SeekableReadStream &) {
	Log::debug("%s doesn't have a supported embedded screenshot", filename.c_str());
	return image::ImagePtr();
}

bool Format::save(const RawVolume* volume, const core::String &filename, io::SeekableWriteStream& stream) {
	VoxelVolumes volumes;
	volumes.push_back(VoxelVolume(const_cast<RawVolume*>(volume)));
	return saveGroups(volumes, filename, stream);
}

MeshExporter::MeshExt::MeshExt(voxel::Mesh *_mesh, const core::String &_name) :
		mesh(_mesh), name(_name) {
}

bool MeshExporter::loadGroups(const core::String &filename, io::SeekableReadStream& file, VoxelVolumes& volumes) {
	Log::debug("Meshes can't get voxelized yet");
	return false;
}

bool MeshExporter::saveGroups(const VoxelVolumes& volumes, const core::String &filename, io::SeekableWriteStream& stream) {
	const bool mergeQuads = core::Var::get(cfg::VoxformatMergequads, "true", core::CV_NOPERSIST)->boolVal();
	const bool reuseVertices = core::Var::get(cfg::VoxformatReusevertices, "true", core::CV_NOPERSIST)->boolVal();
	const bool ambientOcclusion = core::Var::get(cfg::VoxformatAmbientocclusion, "false", core::CV_NOPERSIST)->boolVal();
	const float scale = core::Var::get(cfg::VoxformatScale, "1.0", core::CV_NOPERSIST)->floatVal();
	const bool quads = core::Var::get(cfg::VoxformatQuads, "true", core::CV_NOPERSIST)->boolVal();
	const bool withColor = core::Var::get(cfg::VoxformatWithcolor, "true", core::CV_NOPERSIST)->boolVal();
	const bool withTexCoords = core::Var::get(cfg::VoxformatWithtexcoords, "true", core::CV_NOPERSIST)->boolVal();

	Meshes meshes;
	for (const VoxelVolume& v : volumes) {
		voxel::Mesh *mesh = new voxel::Mesh();
		voxel::Region region = v.volume->region();
		region.shiftUpperCorner(1, 1, 1);
		voxel::extractCubicMesh(v.volume, region, mesh, voxel::IsQuadNeeded(), glm::ivec3(0), mergeQuads, reuseVertices, ambientOcclusion);
		meshes.emplace_back(mesh, v.name);
	}
	Log::debug("Save meshes");
	const bool state = saveMeshes(meshes, filename, stream, scale, quads, withColor, withTexCoords);
	for (MeshExt& meshext : meshes) {
		delete meshext.mesh;
	}
	return state;
}

}
