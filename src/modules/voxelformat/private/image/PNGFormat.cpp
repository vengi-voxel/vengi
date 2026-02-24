/**
 * @file
 */

#include "PNGFormat.h"
#include "app/Async.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "io/Archive.h"
#include "io/FilesystemEntry.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxelutil/ImageUtils.h"

namespace voxelformat {

#define MaxHeightmapWidth 4096
#define MaxHeightmapHeight 4096

static int extractLayerFromFilename(const core::String &filename) {
	core::String name = core::string::extractFilename(filename);
	size_t sep = name.rfind('-');
	if (sep == core::String::npos) {
		Log::error("Invalid image name %s", name.c_str());
		return -1;
	}
	const int layer = name.substr(sep + 1).toInt();
	return layer;
}

static bool hasSameBasename(const core::String &originalFilename, const core::String &layerFilename) {
	core::String o = core::string::extractFilename(originalFilename);
	size_t n = o.rfind("-");
	if (n == core::String::npos) {
		return false;
	}
	o = o.substr(0, n);
	core::String l = core::string::extractFilename(layerFilename);
	n = l.rfind("-");
	if (n == core::String::npos) {
		return false;
	}
	l = l.substr(0, n);
	return core::string::iequals(l, o);
}

bool PNGFormat::importSlices(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							 const io::ArchiveFiles &entities) const {
	const core::String filename = entities.front().fullPath;
	Log::debug("Use %s as reference image", filename.c_str());
	image::ImagePtr referenceImage = image::loadImage(filename);
	if (!referenceImage || !referenceImage->isLoaded()) {
		Log::error("Failed to load first image as reference %s", filename.c_str());
		return false;
	}
	const int imageWidth = referenceImage->width();
	const int imageHeight = referenceImage->height();
	referenceImage.release();
	int minsZ = 1000000;
	int maxsZ = -1000000;

	core::DynamicArray<const io::FilesystemEntry*> filteredEntites;
	filteredEntites.reserve(entities.size());
	for (const auto &entity : entities) {
		const core::String &layerFilename = entity.fullPath;
		if (!hasSameBasename(filename, layerFilename)) {
			continue;
		}
		if (!io::isImage(layerFilename)) {
			continue;
		}
		const int layer = extractLayerFromFilename(layerFilename);
		if (layer < 0) {
			Log::error("Failed to extract layer from filename %s", layerFilename.c_str());
			continue;
		}
		minsZ = glm::min(minsZ, layer);
		maxsZ = glm::max(maxsZ, layer);
		filteredEntites.push_back(&entity);
	}

	voxel::Region region(0, 0, minsZ, imageWidth - 1, imageHeight - 1, maxsZ);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);

	palette::PaletteLookup palLookup(palette);
	auto fn = [&filteredEntites, &palLookup, &palette, &volume, imageHeight, imageWidth, this] (int start, int end) {
		for (int i = start; i < end; ++i) {
			const auto &entity = *filteredEntites[i];
			const core::String &layerFilename = entity.fullPath;
			const image::ImagePtr &image = image::loadImage(layerFilename);
			if (!image || !image->isLoaded()) {
				Log::error("Failed to load image %s", layerFilename.c_str());
				continue;
			}
			if (imageWidth != image->width() || imageHeight != image->height()) {
				Log::error("Image %s has different dimensions than the first image (%d:%d) vs (%d:%d)",
						layerFilename.c_str(), image->width(), image->height(), imageWidth, imageHeight);
				continue;
			}
			const int layer = extractLayerFromFilename(layerFilename);
			if (layer < 0) {
				Log::error("Failed to extract layer from filename %s", layerFilename.c_str());
				continue;
			}
			Log::debug("Import layer %i of image %s", layer, layerFilename.c_str());

			voxel::RawVolume::Sampler sampler(volume);
			sampler.setPosition(0, 0, layer);
			for (int y = 0; y < imageHeight; ++y) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (int x = 0; x < imageWidth; ++x) {
					const color::RGBA &color = flattenRGB(image->colorAt(x, y));
					if (color.a == 0) {
						sampler2.movePositiveX();
						continue;
					}
					const int palIdx = palLookup.findClosestIndex(color);
					sampler2.setVoxel(voxel::createVoxel(palette, palIdx));
					sampler2.movePositiveX();
				}
				sampler.movePositiveY();
			}
		}
	};
	app::for_parallel(0, filteredEntites.size(), fn);
	if (sceneGraph.emplace(core::move(node)) == InvalidNodeId) {
		Log::error("Failed to add node to scene graph");
		return false;
	}
	return true;
}

bool PNGFormat::importAsHeightmap(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
								  const core::String &filename, const io::ArchivePtr &archive) const {
	image::ImagePtr image = image::loadImage(filename);
	if (image->width() > MaxHeightmapWidth || image->height() >= MaxHeightmapHeight) {
		Log::warn("Skip creating heightmap - image dimensions exceeds the max allowed boundaries");
		return false;
	}
	const bool coloredHeightmap = image->components() == 4 && !image->isGrayScale();
	const int maxHeight = voxelutil::importHeightMaxHeight(image, coloredHeightmap);
	if (maxHeight <= 0) {
		Log::error("There is no height in either the red channel or the alpha channel");
		return false;
	}
	if (maxHeight == 1) {
		Log::warn("There is no height value in the image - it is imported as flat plane");
	}
	Log::info("Generate from heightmap (%i:%i) with max height of %i", image->width(), image->height(), maxHeight);
	voxel::Region region(0, 0, 0, image->width() - 1, maxHeight - 1, image->height() - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	voxel::RawVolumeWrapper wrapper(volume);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	const voxel::Voxel dirtVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const uint8_t minHeight = core::Var::getVar(cfg::VoxformatImageHeightmapMinHeight)->intVal();
	if (coloredHeightmap) {
		voxelutil::importColoredHeightmap(wrapper, palette, image, dirtVoxel, minHeight, false);
	} else {
		const voxel::Voxel grassVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 2);
		voxelutil::importHeightmap(wrapper, image, dirtVoxel, grassVoxel, minHeight, false);
	}
	node.setPalette(palette);
	node.setVolume(volume, true);
	node.setName(core::string::extractFilename(filename));
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool PNGFormat::importAsVolume(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							   const core::String &filename, const io::ArchivePtr &archive) const {
	const image::ImagePtr &image = image::loadImage(filename);
	const int maxDepth = core::Var::getVar(cfg::VoxformatImageVolumeMaxDepth)->intVal();
	const bool bothSides = core::Var::getVar(cfg::VoxformatImageVolumeBothSides)->boolVal();
	const core::String &depthMapFilename = voxelutil::getDefaultDepthMapFile(filename);
	core::ScopedPtr<io::SeekableReadStream> depthMapStream(archive->readStream(depthMapFilename));
	const image::ImagePtr &depthMapImage = image::loadImage(depthMapFilename, *depthMapStream, depthMapStream->size());
	voxel::RawVolume *v;
	if (depthMapImage && depthMapImage->isLoaded()) {
		Log::debug("Found depth map %s", depthMapFilename.c_str());
		v = voxelutil::importAsVolume(image, depthMapImage, palette, maxDepth, bothSides);
	} else {
		Log::debug("Could not find a depth map for %s with the name %s", filename.c_str(), depthMapFilename.c_str());
		v = voxelutil::importAsVolume(image, palette, maxDepth, bothSides);
	}
	if (v == nullptr) {
		Log::warn("Failed to import image as volume: '%s'", image->name().c_str());
		return false;
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(v, true);
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool PNGFormat::importAsPlane(scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							  const core::String &filename, const io::ArchivePtr &archive) const {
	image::ImagePtr image = image::loadImage(filename);
	voxel::RawVolume *v = voxelutil::importAsPlane(image, palette);
	if (v == nullptr) {
		Log::warn("Failed to import image as plane: '%s'", image->name().c_str());
		return false;
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(v, true);
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool PNGFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							   const LoadContext &ctx) {
	const int type = core::Var::getVar(cfg::VoxformatImageImportType)->intVal();
	if (type == ImageType::Heightmap) {
		return importAsHeightmap(sceneGraph, palette, filename, archive);
	}
	if (type == ImageType::Volume) {
		return importAsVolume(sceneGraph, palette, filename, archive);
	}

	core::String basename = core::string::extractFilename(filename);
	const core::String &directory = core::string::extractDir(filename);
	size_t sep = basename.rfind('-');
	if (sep != core::String::npos) {
		basename = basename.substr(0, sep);
	}
	Log::debug("Base name for image layer import is: %s", basename.c_str());

	io::ArchiveFiles entities;
	archive->list(directory, entities, core::String::format("%s-*.png", basename.c_str()));
	if (entities.empty()) {
		io::FilesystemEntry val = io::createFilesystemEntry(filename);
		entities.push_back(val);
	}
	Log::debug("Found %i images for import", (int)entities.size());

	if (entities.size() > 1u) {
		return importSlices(sceneGraph, palette, entities);
	}
	return importAsPlane(sceneGraph, palette, filename, archive);
}

size_t PNGFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							  const LoadContext &ctx) {
	const image::ImagePtr &image = image::loadImage(filename);
	const int type = core::Var::getVar(cfg::VoxformatImageImportType)->intVal();
	if (type == ImageType::Heightmap) {
		image->makeOpaque();
	}

	if (!palette.createPalette(image, palette)) {
		Log::error("Failed to create palette from image %s", filename.c_str());
		return 0;
	}
	Log::debug("Created palette with %i colors from image %s", palette.colorCount(), filename.c_str());
	return palette.colorCount();
}

bool PNGFormat::saveThumbnail(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							   const io::ArchivePtr &archive, const SaveContext &savectx) const {
	Log::debug("Create thumbnail for %s", filename.c_str());
	ThumbnailContext ctx;
	// if we are using the default thumbnailer, we want it to be pixel perfect - and here we
	// use the internal knowledge that -1 simply prevents a scaling of the resulting image.
	if (savectx.thumbnailCreator == nullptr) {
		ctx.outputSize = {-1, -1};
	}
	const image::ImagePtr &image = createThumbnail(sceneGraph, savectx.thumbnailCreator, ctx);
	if (!image || !image->isLoaded()) {
		Log::error("Failed to create thumbnail for %s", filename.c_str());
		return false;
	}
	core::ScopedPtr<io::SeekableWriteStream> writeStream(archive->writeStream(filename));
	if (!writeStream) {
		Log::error("Failed to open write stream for %s", filename.c_str());
		return false;
	}
	if (!image->writePNG(*writeStream)) {
		Log::error("Failed to write slice image %s", filename.c_str());
		return false;
	}
	return true;
}

bool PNGFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	const int type = core::Var::getVar(cfg::VoxformatImageSaveType)->intVal();
	if (type == ImageType::Heightmap) {
		return saveHeightmaps(sceneGraph, filename, archive);
	}
	if (type == ImageType::Volume) {
		return saveVolumes(sceneGraph, filename, archive);
	}
	if (type == ImageType::Thumbnail) {
		return saveThumbnail(sceneGraph, filename, archive, ctx);
	}

	return saveSlices(sceneGraph, filename, archive);
}

bool PNGFormat::saveHeightmaps(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							   const io::ArchivePtr &archive) const {
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->value;
		if (!node.isAnyModelNode()) {
			continue;
		}
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		core_assert(volume != nullptr);
		const voxel::Region &region = volume->region();
		// TODO: VOXELFORMAT: make max height configurable
		const float heightScale = 256.0f / (float)region.getHeightInVoxels();
		const palette::Palette &palette = node.palette();
		const core::String &uuidStr = node.uuid().str();
		const core::String name = core::String::format("%s-%s.png", core::string::stripExtension(filename).c_str(),
												 uuidStr.c_str());
		image::Image image(name, 4);
		image.resize(region.getWidthInVoxels(), region.getDepthInVoxels());
		app::for_parallel(region.getLowerZ(), region.getUpperZ() + 1, [&image, volume, &region, &palette, heightScale] (int start, int end) {
			voxel::RawVolume::Sampler sampler(volume);
			sampler.setPosition(region.getLowerX(), region.getUpperY(), start);
			for (int z = start; z < end; ++z) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (int y = region.getUpperY(); y >= region.getLowerY(); --y) {
						if (isBlocked(sampler3.voxel().getMaterial())) {
							color::RGBA color = palette.color(sampler3.voxel().getColor());
							color.a = (y + 1) * heightScale;
							image.setColor(color, x - region.getLowerX(),
										z - region.getLowerZ());
							break;
						}
						sampler3.moveNegativeY();
					}
					sampler2.movePositiveX();
				}
				sampler.movePositiveZ();
			}
		});
		core::ScopedPtr<io::SeekableWriteStream> writeStream(archive->writeStream(name));
		if (!writeStream) {
			Log::error("Failed to open write stream for %s", name.c_str());
			return false;
		}
		if (!image.writePNG(*writeStream)) {
			Log::error("Failed to write image %s", name.c_str());
			return false;
		}
		Log::debug("Saved heightmap image %s", name.c_str());
	}
	return true;
}

bool PNGFormat::saveVolumes(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							const io::ArchivePtr &archive) const {
	Log::error("Saving volumes as PNG is not supported");
	return false;
}

bool PNGFormat::saveSlices(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive) const {
	const core::String &basename = core::string::stripExtension(filename);
	for (const auto &e : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = e->value;
		if (!node.isAnyModelNode()) {
			continue;
		}
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		core_assert(volume != nullptr);
		const voxel::Region &region = volume->region();
		const palette::Palette &palette = node.palette();
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			const core::String &uuidStr = node.uuid().str();
			const core::String &layerFilename =
				core::String::format("%s-%s-%i.png", basename.c_str(), uuidStr.c_str(), z);
			image::Image image(layerFilename);
			core::Buffer<color::RGBA> rgba;
			rgba.resize(region.getWidthInVoxels() * region.getHeightInVoxels());
			bool empty = true;
			for (int y = region.getUpperY(); y >= region.getLowerY(); --y) {
				for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
					const voxel::Voxel &v = volume->voxel(x, y, z);
					if (voxel::isAir(v.getMaterial())) {
						continue;
					}
					const color::RGBA color = palette.color(v.getColor());
					const int idx = (region.getUpperY() - y) * region.getWidthInVoxels() + (x - region.getLowerX());
					rgba[idx] = color;
					empty = false;
				}
			}
			if (empty) {
				// skip empty slices
				continue;
			}

			if (!image.loadRGBA((const uint8_t *)rgba.data(), region.getWidthInVoxels(), region.getHeightInVoxels())) {
				Log::error("Failed to load sliced rgba data %s", layerFilename.c_str());
				return false;
			}
			core::ScopedPtr<io::SeekableWriteStream> writeStream(archive->writeStream(layerFilename));
			if (!writeStream) {
				Log::error("Failed to open write stream for %s", layerFilename.c_str());
				return false;
			}
			if (!image.writePNG(*writeStream)) {
				Log::error("Failed to write slice image %s", layerFilename.c_str());
				return false;
			}
		}
	}
	return true;
}

} // namespace voxelformat
