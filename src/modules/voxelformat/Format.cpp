/**
 * @file
 */

#include "Format.h"
#include "app/App.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "voxel/MaterialColor.h"
#include "VolumeFormat.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/Color.h"
#include "core/StringUtil.h"
#include "math/Math.h"
#include "voxel/Mesh.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/SceneGraphUtil.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeSplitter.h"
#include "voxelutil/VoxelUtil.h"
#include <limits>

namespace voxelformat {

core::String Format::stringProperty(const SceneGraphNode* node, const core::String &name, const core::String &defaultVal) {
	if (node == nullptr) {
		return defaultVal;
	}
	if (!node->properties().hasKey(name)) {
		return defaultVal;
	}
	return node->property(name);
}

bool Format::boolProperty(const SceneGraphNode* node, const core::String &name, bool defaultVal) {
	if (node == nullptr) {
		return defaultVal;
	}
	if (!node->properties().hasKey(name)) {
		return defaultVal;
	}
	return core::string::toBool(node->property(name));
}

float Format::floatProperty(const SceneGraphNode* node, const core::String &name, float defaultVal) {
	if (node == nullptr) {
		return defaultVal;
	}
	if (!node->properties().hasKey(name)) {
		return defaultVal;
	}
	return core::string::toFloat(node->property(name));
}

image::ImagePtr Format::createThumbnail(const SceneGraph& sceneGraph, ThumbnailCreator thumbnailCreator, const ThumbnailContext &ctx) {
	if (thumbnailCreator == nullptr) {
		return image::ImagePtr();
	}

	return thumbnailCreator(sceneGraph, ctx);
}

// TODO: split is destroying groups
void Format::splitVolumes(const SceneGraph& srcSceneGraph, SceneGraph& destSceneGraph, const glm::ivec3 &maxSize, bool crop) {
	destSceneGraph.reserve(srcSceneGraph.size());
	for (SceneGraphNode &node : srcSceneGraph) {
		if (stopExecution()) {
			break;
		}
		const voxel::Region& region = node.region();
		if (!region.isValid()) {
			Log::debug("invalid region for node %i", node.id());
			continue;
		}
		if (glm::all(glm::lessThanEqual(region.getDimensionsInVoxels(), maxSize))) {
			SceneGraphNode newNode;
			copyNode(node, newNode, true);
			destSceneGraph.emplace(core::move(newNode));
			continue;
		}
		core::DynamicArray<voxel::RawVolume *> rawVolumes;
		voxelutil::splitVolume(node.volume(), maxSize, rawVolumes);
		for (voxel::RawVolume *v : rawVolumes) {
			SceneGraphNode newNode;
			if (crop) {
				voxel::RawVolume *cv = voxelutil::cropVolume(v);
				delete v;
				v = cv;
			}
			copyNode(node, newNode, false);
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

size_t Format::loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) {
	return 0;
}

size_t PaletteFormat::loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) {
	SceneGraph sceneGraph;
	loadGroupsPalette(filename, stream, sceneGraph, palette);
	return palette.colorCount;
}

image::ImagePtr Format::loadScreenshot(const core::String &filename, io::SeekableReadStream &) {
	Log::debug("%s doesn't have a supported embedded screenshot", filename.c_str());
	return image::ImagePtr();
}

glm::ivec3 Format::maxSize() const {
	return glm::ivec3(-1);
}

bool Format::singleVolume() const {
	return core::Var::getSafe(cfg::VoxformatMerge)->boolVal();
}

bool Format::save(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, ThumbnailCreator thumbnailCreator) {
	bool needsSplit = false;
	const glm::ivec3 maxsize = maxSize();
	if (maxsize.x > 0 && maxsize.y > 0 && maxsize.z > 0) {
		for (SceneGraphNode &node : sceneGraph) {
			const voxel::Region& region = node.region();
			const glm::ivec3 &maxs = region.getDimensionsInVoxels();
			if (glm::all(glm::lessThanEqual(maxs, maxsize))) {
				continue;
			}
			Log::debug("Need to split node %s because it exceeds the max size (%i:%i:%i)", node.name().c_str(), maxs.x, maxs.y, maxs.z);
			needsSplit = true;
			break;
		}
	}

	if (needsSplit && singleVolume()) {
		Log::error("Failed to save. This format can't be used to save the scene graph");
		return false;
	}

	if (singleVolume()) {
		SceneGraph::MergedVolumePalette merged = sceneGraph.merge(true);
		SceneGraph mergedSceneGraph(2);
		SceneGraphNode mergedNode(SceneGraphNodeType::Model);
		mergedNode.setVolume(merged.first, true);
		mergedNode.setPalette(merged.second);
		mergedSceneGraph.emplace(core::move(mergedNode));
		return saveGroups(mergedSceneGraph, filename, stream, thumbnailCreator);
	}

	if (needsSplit) {
		SceneGraph newSceneGraph;
		splitVolumes(sceneGraph, newSceneGraph, maxsize);
		return saveGroups(newSceneGraph, filename, stream, thumbnailCreator);
	}
	return saveGroups(sceneGraph, filename, stream, thumbnailCreator);
}

bool Format::load(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	return loadGroups(filename, stream, sceneGraph);
}

bool Format::stopExecution() {
	return app::App::getInstance()->shouldQuit();
}

bool PaletteFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	voxel::Palette palette;
	if (!loadGroupsPalette(filename, stream, sceneGraph, palette)) {
		return false;
	}
	sceneGraph.updateTransforms();
	return true;
}

bool PaletteFormat::save(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, ThumbnailCreator thumbnailCreator) {
	if (onlyOnePalette()) {
		// TODO: reduce palettes to one - construct a new scene graph
	}
	return Format::save(sceneGraph, filename, stream, thumbnailCreator);
}

RGBAFormat::RGBAFormat() {
	_flattenFactor = core::Var::getSafe(cfg::VoxformatRGBFlattenFactor)->intVal();
}

core::RGBA RGBAFormat::flattenRGB(core::RGBA rgba) const {
	return core::Color::flattenRGB(rgba.r, rgba.g, rgba.b, rgba.a, _flattenFactor);
}

core::RGBA RGBAFormat::flattenRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a) const {
	return core::Color::flattenRGB(r, g, b, a, _flattenFactor);
}

bool RGBAFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	voxel::Palette palette;
	const bool createPalette = core::Var::get(cfg::VoxelCreatePalette);
	if (createPalette) {
		const int64_t resetToPos = stream.pos();
		if (loadPalette(filename, stream, palette) <= 0) {
			palette = voxel::getPalette();
		}
		stream.seek(resetToPos);
	} else {
		palette = voxel::getPalette();
	}
	if (!loadGroupsRGBA(filename, stream, sceneGraph, palette)) {
		return false;
	}
	sceneGraph.updateTransforms();
	return true;
}

}
