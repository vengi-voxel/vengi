/**
 * @file
 */

#include "Format.h"
#include "VolumeFormat.h"
#include "app/App.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/GLMConst.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "math/Math.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeSplitter.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxelutil/VoxelUtil.h"
#include <limits>

namespace voxelformat {

core::String Format::stringProperty(const scenegraph::SceneGraphNode *node, const core::String &name,
									const core::String &defaultVal) {
	if (node == nullptr) {
		return defaultVal;
	}
	if (!node->properties().hasKey(name)) {
		return defaultVal;
	}
	return node->property(name);
}

bool Format::boolProperty(const scenegraph::SceneGraphNode *node, const core::String &name, bool defaultVal) {
	if (node == nullptr) {
		return defaultVal;
	}
	if (!node->properties().hasKey(name)) {
		return defaultVal;
	}
	return core::string::toBool(node->property(name));
}

float Format::floatProperty(const scenegraph::SceneGraphNode *node, const core::String &name, float defaultVal) {
	if (node == nullptr) {
		return defaultVal;
	}
	if (!node->properties().hasKey(name)) {
		return defaultVal;
	}
	return core::string::toFloat(node->property(name));
}

image::ImagePtr Format::createThumbnail(const scenegraph::SceneGraph &sceneGraph, ThumbnailCreator thumbnailCreator,
										const ThumbnailContext &ctx) {
	if (thumbnailCreator == nullptr) {
		return image::ImagePtr();
	}

	return thumbnailCreator(sceneGraph, ctx);
}

// TODO: split is destroying groups
void Format::splitVolumes(const scenegraph::SceneGraph &srcSceneGraph, scenegraph::SceneGraph &destSceneGraph,
						  const glm::ivec3 &maxSize, bool crop) {
	destSceneGraph.reserve(srcSceneGraph.size());
	for (auto iter = srcSceneGraph.beginModel(); iter != srcSceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (stopExecution()) {
			break;
		}
		const voxel::Region &region = node.region();
		if (!region.isValid()) {
			Log::debug("invalid region for node %i", node.id());
			continue;
		}
		if (glm::all(glm::lessThanEqual(region.getDimensionsInVoxels(), maxSize))) {
			scenegraph::SceneGraphNode newNode;
			copyNode(node, newNode, true);
			destSceneGraph.emplace(core::move(newNode));
			Log::debug("No split needed for node '%s'", node.name().c_str());
			continue;
		}
		Log::debug("Split needed for node '%s'", node.name().c_str());
		core::DynamicArray<voxel::RawVolume *> rawVolumes;
		voxelutil::splitVolume(node.volume(), maxSize, rawVolumes);
		for (voxel::RawVolume *v : rawVolumes) {
			scenegraph::SceneGraphNode newNode;
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

void Format::calcMinsMaxs(const voxel::Region &region, const glm::ivec3 &maxSize, glm::ivec3 &mins,
						  glm::ivec3 &maxs) const {
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

size_t Format::loadPalette(const core::String &, io::SeekableReadStream &, voxel::Palette &, const LoadContext &) {
	return 0;
}

size_t PaletteFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
								  const LoadContext &ctx) {
	scenegraph::SceneGraph sceneGraph;
	loadGroupsPalette(filename, stream, sceneGraph, palette, ctx);
	return palette.size();
}

image::ImagePtr Format::loadScreenshot(const core::String &filename, io::SeekableReadStream &, const LoadContext &) {
	Log::debug("%s doesn't have a supported embedded screenshot", filename.c_str());
	return image::ImagePtr();
}

glm::ivec3 Format::maxSize() const {
	return glm::ivec3(-1);
}

bool Format::singleVolume() const {
	return core::Var::getSafe(cfg::VoxformatMerge)->boolVal();
}

bool Format::save(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
				  io::SeekableWriteStream &stream, const SaveContext &ctx) {
	bool needsSplit = false;
	const glm::ivec3 maxsize = maxSize();
	if (maxsize.x > 0 && maxsize.y > 0 && maxsize.z > 0) {
		for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
			const scenegraph::SceneGraphNode &node = *iter;
			const voxel::Region &region = node.region();
			const glm::ivec3 &maxs = region.getDimensionsInVoxels();
			if (glm::all(glm::lessThanEqual(maxs, maxsize))) {
				continue;
			}
			Log::debug("Need to split node %s because it exceeds the max size (%i:%i:%i)", node.name().c_str(), maxs.x,
					   maxs.y, maxs.z);
			needsSplit = true;
			break;
		}
	}

	if (needsSplit && singleVolume()) {
		Log::error("Failed to save. This format can't be used to save the scene graph");
		return false;
	}

	if (singleVolume() && sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels) > 1) {
		scenegraph::SceneGraph::MergedVolumePalette merged = sceneGraph.merge(true);
		scenegraph::SceneGraph mergedSceneGraph(2);
		scenegraph::SceneGraphNode mergedNode(scenegraph::SceneGraphNodeType::Model);
		mergedNode.setVolume(merged.first, true);
		mergedNode.setPalette(merged.second);
		mergedSceneGraph.emplace(core::move(mergedNode));
		return saveGroups(mergedSceneGraph, filename, stream, ctx);
	}

	if (needsSplit) {
		scenegraph::SceneGraph newSceneGraph;
		splitVolumes(sceneGraph, newSceneGraph, maxsize);
		return saveGroups(newSceneGraph, filename, stream, ctx);
	}
	return saveGroups(sceneGraph, filename, stream, ctx);
}

bool Format::load(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
				  const LoadContext &ctx) {
	return loadGroups(filename, stream, sceneGraph, ctx);
}

bool Format::stopExecution() {
	return app::App::getInstance()->shouldQuit();
}

bool PaletteFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	voxel::Palette palette;
	if (!loadGroupsPalette(filename, stream, sceneGraph, palette, ctx)) {
		return false;
	}
	sceneGraph.updateTransforms();
	return true;
}

bool PaletteFormat::save(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						 io::SeekableWriteStream &stream, const SaveContext &ctx) {
	if (onlyOnePalette() && sceneGraph.hasMoreThanOnePalette()) {
		Log::debug("Need to merge palettes before saving");
		const voxel::Palette &palette = sceneGraph.mergePalettes(true, emptyPaletteIndex());
		scenegraph::SceneGraph newSceneGraph;
		scenegraph::copySceneGraph(newSceneGraph, sceneGraph);
		for (auto iter = newSceneGraph.beginAllModels(); iter != newSceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			node.remapToPalette(palette);
			node.setPalette(palette);
		}
		return Format::save(newSceneGraph, filename, stream, ctx);
	} else if (emptyPaletteIndex() >= 0 && emptyPaletteIndex() < voxel::PaletteMaxColors) {
		Log::debug("Need to convert voxels to a palette that has %i as an empty slot", emptyPaletteIndex());
		scenegraph::SceneGraph newSceneGraph;
		scenegraph::copySceneGraph(newSceneGraph, sceneGraph);
		for (auto iter = newSceneGraph.beginModel(); iter != newSceneGraph.end(); ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			voxel::Palette palette = node.palette();
			if (palette.color(emptyPaletteIndex()).a > 0) {
				Log::debug("Need to replace %i", emptyPaletteIndex());
				if (palette.colorCount() < voxel::PaletteMaxColors) {
					Log::debug("Shift colors in palettes to make slot %i empty", emptyPaletteIndex());
					for (int i = palette.colorCount(); i >= emptyPaletteIndex(); --i) {
						palette.color(i) = palette.color(i - 1);
						palette.glowColor(i) = palette.glowColor(i - 1);
					}
					if (emptyPaletteIndex() <= palette.colorCount()) {
						palette.changeSize(1);
					}
					voxel::RawVolume *v = node.volume();
					voxelutil::visitVolume(
						*v, [v, skip = emptyPaletteIndex()](int x, int y, int z, const voxel::Voxel &voxel) {
							if (voxel.getColor() >= skip) {
								v->setVoxel(x, y, z, voxel::createVoxel(voxel.getMaterial(), voxel.getColor() + 1));
							}
						});
				} else {
					uint8_t replacement = palette.findReplacement(emptyPaletteIndex());
					Log::debug("Looking for a similar color in the palette: %d", replacement);
					if (replacement != emptyPaletteIndex()) {
						Log::debug("Replace %i with %i", emptyPaletteIndex(), replacement);
						voxel::RawVolume *v = node.volume();
						voxelutil::visitVolume(
							*v, [v, replaceFrom = emptyPaletteIndex(), replaceTo = replacement](int x, int y, int z, const voxel::Voxel &voxel) {
								if (voxel.getColor() == replaceFrom) {
									v->setVoxel(x, y, z, voxel::createVoxel(voxel.getMaterial(), replaceTo));
								}
							});
					}
				}
				node.setPalette(palette);
			} else {
				node.remapToPalette(node.palette(), emptyPaletteIndex());
			}
		}
		return Format::save(newSceneGraph, filename, stream, ctx);
	}
	return Format::save(sceneGraph, filename, stream, ctx);
}

Format::Format() {
	_flattenFactor = core::Var::getSafe(cfg::VoxformatRGBFlattenFactor)->intVal();
}

core::RGBA Format::flattenRGB(core::RGBA rgba) const {
	return core::Color::flattenRGB(rgba.r, rgba.g, rgba.b, rgba.a, _flattenFactor);
}

core::RGBA Format::flattenRGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a) const {
	return core::Color::flattenRGB(r, g, b, a, _flattenFactor);
}

bool RGBAFormat::loadGroups(const core::String &filename, io::SeekableReadStream &stream,
							scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	voxel::Palette palette;
	const bool createPalette = core::Var::get(cfg::VoxelCreatePalette);
	if (createPalette) {
		const int64_t resetToPos = stream.pos();
		if (loadPalette(filename, stream, palette, ctx) <= 0) {
			palette = voxel::getPalette();
		}
		stream.seek(resetToPos);
	} else {
		palette = voxel::getPalette();
	}
	if (!loadGroupsRGBA(filename, stream, sceneGraph, palette, ctx)) {
		return false;
	}
	sceneGraph.updateTransforms();
	return true;
}

} // namespace voxelformat
