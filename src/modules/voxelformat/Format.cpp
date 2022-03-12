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
#include "voxelformat/SceneGraph.h"
#include "voxelutil/VolumeSplitter.h"
#include "voxelutil/VoxelUtil.h"
#include <limits>

namespace voxel {

uint8_t Format::convertPaletteIndex(uint32_t paletteIndex) const {
	if (paletteIndex >= (uint32_t)_palette.colorCount) {
		if (_palette.colorCount > 0) {
			return paletteIndex % _palette.colorCount;
		}
		return paletteIndex % _paletteMapping.size();
	}
	return _paletteMapping[paletteIndex];
}

uint8_t Format::findClosestIndex(const glm::vec4& color) const {
	const voxel::Palette &palette = voxel::getPalette();
	return palette.getClosestMatch(color);
}

uint8_t Format::findClosestIndex(uint32_t rgba) const {
	const voxel::Palette &palette = voxel::getPalette();
	return palette.getClosestMatch(rgba);
}

static inline glm::vec4 transform(const glm::mat4x4 &mat, const glm::vec3 &pos, const glm::vec4 &pivot) {
	return glm::floor(mat * (glm::vec4((float)pos.x + 0.5f, (float)pos.y + 0.5f, (float)pos.z + 0.5f, 1.0f) - pivot));
}

voxel::RawVolume* Format::transformVolume(const SceneGraphTransform &t, const voxel::RawVolume *in) const {
	const glm::mat4 &mat = t.mat;
	const glm::vec3 &pivotNormalized = t.normalizedPivot;
	const voxel::Region &inRegion = in->region();
	const glm::ivec3 &inMins = inRegion.getLowerCorner();
	const glm::ivec3 &inMaxs = inRegion.getUpperCorner();
	const glm::vec4 pivot(glm::floor(pivotNormalized * glm::vec3(inRegion.getDimensionsInVoxels())), 0.0f);
	const glm::vec4 tmins(transform(mat, inMins, pivot));
	const glm::vec4 tmaxs(transform(mat, inMaxs, pivot));
	const glm::ivec3 rmins = glm::min(tmins, tmaxs);
	const glm::ivec3 rmaxs = glm::max(tmins, tmaxs);
	const voxel::Region outRegion(rmins, rmaxs);
	Log::debug("transform volume");
	Log::debug("* normalized pivot: %f:%f:%f", pivotNormalized.x, pivotNormalized.y, pivotNormalized.z);
	Log::debug("* pivot: %f:%f:%f", pivot.x, pivot.y, pivot.z);
	Log::debug("* mins: %i:%i:%i, maxs: %i:%i:%i", inMins.x, inMins.y, inMins.z, inMaxs.x, inMaxs.y, inMaxs.z);
	Log::debug("* transformed region: mins: %i:%i:%i, maxs: %i:%i:%i", rmins.x, rmins.y, rmins.z, rmaxs.x, rmaxs.y, rmaxs.z);
	voxel::RawVolume *v = new voxel::RawVolume(outRegion);
	voxel::RawVolume::Sampler inSampler(in);
	for (int z = inMins.z; z <= inMaxs.z; ++z) {
		for (int y = inMins.y; y <= inMaxs.y; ++y) {
			inSampler.setPosition(0, y, z);
			for (int x = inMins.x; x <= inMaxs.x; ++x) {
				const voxel::Voxel voxel = inSampler.voxel();
				inSampler.movePositiveX();
				if (voxel::isAir(voxel.getMaterial())) {
					continue;
				}
				const glm::ivec3 pos(transform(mat, inSampler.position(), pivot));
				v->setVoxel(pos, voxel);
			}
		}
	}
	return v;
}

void Format::splitVolumes(const SceneGraph& srcSceneGraph, SceneGraph& destSceneGraph, const glm::ivec3 &maxSize) {
	destSceneGraph.reserve(srcSceneGraph.size());
	for (SceneGraphNode &node : srcSceneGraph) {
		const voxel::Region& region = node.region();
		if (glm::all(glm::lessThan(region.getDimensionsInVoxels(), maxSize))) {
			SceneGraphNode newNode;
			newNode.setVolume(new voxel::RawVolume(node.volume()), true);
			newNode.setName(node.name());
			newNode.setVisible(node.visible());
			newNode.setKeyFrames(node.keyFrames());
			destSceneGraph.emplace(core::move(newNode));
			continue;
		}
		core::DynamicArray<voxel::RawVolume *> rawVolumes;
		voxel::splitVolume(node.volume(), maxSize, rawVolumes);
		for (voxel::RawVolume *v : rawVolumes) {
			SceneGraphNode newNode;
			newNode.setVolume(v, true);
			destSceneGraph.emplace(core::move(newNode));
		}
	}
}

bool Format::isEmptyBlock(const voxel::RawVolume *v, const glm::ivec3 &maxSize, int x, int y, int z) const {
	const voxel::Region region(x, y, z, x + maxSize.x - 1, y + maxSize.y - 1, z + maxSize.z - 1);
	return voxelutil::isEmpty(*v, region);
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

RawVolume* Format::merge(const SceneGraph& sceneGraph) const {
	return sceneGraph.merge();
}

RawVolume* Format::load(const core::String &filename, io::SeekableReadStream& stream) {
	SceneGraph sceneGraph;
	if (!loadGroups(filename, stream, sceneGraph)) {
		return nullptr;
	}
	RawVolume* mergedVolume = merge(sceneGraph);
	return mergedVolume;
}

size_t Format::loadPalette(const core::String &filename, io::SeekableReadStream& stream, Palette &palette) {
	SceneGraph sceneGraph;
	loadGroups(filename, stream, sceneGraph);
	palette = _palette;
	return palette.colorCount;
}

image::ImagePtr Format::loadScreenshot(const core::String &filename, io::SeekableReadStream &) {
	Log::debug("%s doesn't have a supported embedded screenshot", filename.c_str());
	return image::ImagePtr();
}

bool Format::save(const RawVolume* volume, const core::String &filename, io::SeekableWriteStream& stream) {
	if (volume == nullptr) {
		return false;
	}
	SceneGraph sceneGraph;
	SceneGraphNode node;
	node.setVolume(volume, false);
	sceneGraph.emplace(core::move(node));
	return saveGroups(sceneGraph, filename, stream);
}

}
