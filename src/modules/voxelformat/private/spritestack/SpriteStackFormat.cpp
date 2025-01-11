/**
 * @file
 */

#include "SpriteStackFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "image/Image.h"
#include "io/ZipArchive.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "video/Texture.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "json/JSON.h"
#include <glm/common.hpp>

namespace voxelformat {

bool SpriteStackFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
										  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
										  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	const io::ArchivePtr &zipArchive = io::openZipArchive(stream);
	if (!zipArchive) {
		Log::error("Failed to open zip archive");
		return false;
	}

	// --- parse slices.json ---
	core::ScopedPtr<io::SeekableReadStream> slicesJson(zipArchive->readStream("slices.json"));
	if (!slicesJson) {
		Log::error("Failed to read slices.json from zip archive");
		return false;
	}
	core::String jsonString;
	slicesJson->readString(slicesJson->remaining(), jsonString);
	nlohmann::json json = nlohmann::json::parse(jsonString, nullptr, false, true);
	if (json.is_discarded()) {
		Log::error("Failed to parse slices.json: %s", jsonString.c_str());
		return false;
	}

	struct SlicesInfo {
		int slices = 0;
		int frames = 1; // frames per row in the atlas (default 1)
		int width = 0;
		int height = 0;
	} info;

	if (json.contains("slices") && json["slices"].is_number_integer()) {
		info.slices = json["slices"].get<int>();
	}
	if (json.contains("frames") && json["frames"].is_number_integer()) {
		info.frames = json["frames"].get<int>();
	}
	if (json.contains("width") && json["width"].is_number_integer()) {
		info.width = json["width"].get<int>();
	}
	if (json.contains("height") && json["height"].is_number_integer()) {
		info.height = json["height"].get<int>();
	}
	if (info.slices <= 0 || info.frames <= 0 || info.width <= 0 || info.height <= 0) {
		Log::error("Invalid slices data: %d slices, %d frames, %d width, %d height", info.slices, info.frames,
				   info.width, info.height);
		return false;
	}

	core::ScopedPtr<io::SeekableReadStream> slicesPng(zipArchive->readStream("slices.png"));
	if (!slicesPng) {
		Log::error("Failed to read slices.png from zip archive");
		return false;
	}

	image::ImagePtr image = image::loadImage("slices.png", *slicesPng);
	if (!image || !image->isLoaded()) {
		Log::error("Failed to load slices.png from zip archive");
		return false;
	}

	if (palette.colorCount() == 0) {
		if (!palette::Palette::createPalette(image, palette, info.width, info.height)) {
			Log::error("Failed to create palette from slices.png");
			return 0u;
		}
	}

	const int imgW = image->width();
	const int imgH = image->height();
	int atlasCols = info.frames;
	if (atlasCols <= 0)
		atlasCols = 1;
	const int atlasRows = (info.slices + atlasCols - 1) / atlasCols;

	const bool matchesAtlas = (atlasCols * info.width == imgW) && (atlasRows * info.height == imgH);
	const bool matchesVerticalStack = (info.width == imgW) && (info.height * info.slices == imgH);
	const bool matchesHorizontalStack = (info.height == imgH) && (info.width * info.slices == imgW);
	const bool matchesSingle = (info.width == imgW) && (info.height == imgH) && info.slices == 1;
	if (!matchesAtlas && !matchesVerticalStack && !matchesHorizontalStack && !matchesSingle) {
		Log::error(
			"slices.png size %dx%d doesn't match expected atlas (%dx%d) or stacks. Trying best-effort extraction.",
			imgW, imgH, atlasCols * info.width, atlasRows * info.height);
		return false;
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(core::String::format("slices_model_%s", filename.c_str()));

	voxel::Region region(0, 0, 0, info.width - 1, info.height - 1, info.slices - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	palette::PaletteLookup palLookup(palette);
	node.setVolume(volume);
	node.setPalette(palette);
	Log::error("Region width: %d, height: %d, depth: %d", region.getWidthInVoxels(), region.getHeightInVoxels(),
			   region.getDepthInVoxels());
	Log::error("Slices: %d, Frames: %d, Width: %d, Height: %d", info.slices, info.frames, info.width, info.height);
	Log::error("atlasCols: %d, atlasRows: %d, imgW: %d, imgH: %d", atlasCols, atlasRows, imgW, imgH);

	// For each slice i, compute sub-image origin in the atlas and iterate pixels:
	for (int i = 0; i < info.slices; ++i) {
		int sx;
		int sy;

		if (matchesVerticalStack) {
			sx = 0;
			sy = i * info.height;
		} else if (matchesHorizontalStack) {
			sx = i * info.width;
			sy = 0;
		} else {
			const int col = i % atlasCols;
			const int row = i / atlasCols;
			sx = col * info.width;
			sy = row * info.height;
		}

		if (sx < 0 || sy < 0 || sx + info.width > imgW || sy + info.height > imgH) {
			Log::error("Slice %d out of bounds in slices.png (sx=%d, sy=%d, w=%d, h=%d, img=%dx%d)", i, sx, sy,
					   info.width, info.height, imgW, imgH);
			return false;
		}

		for (int yy = 0; yy < info.height; ++yy) {
			for (int xx = 0; xx < info.width; ++xx) {
				const int px = sx + xx;
				const int py = sy + yy;

				const core::RGBA color = image->colorAt(px, py);
				// skip fully transparent pixels
				if (color.a == 0) {
					continue;
				}

				const uint8_t palIdx = palLookup.findClosestIndex(color);
				const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, palIdx);
				volume->setVoxel(xx, info.height - 1 - yy, i, voxel);
			}
		}
	}
	if (sceneGraph.emplace(core::move(node)) == InvalidNodeId) {
		Log::error("Failed to add node to scene graph");
		return false;
	}

	return !sceneGraph.empty();
}

size_t SpriteStackFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive,
									  palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return 0u;
	}

	const io::ArchivePtr &zipArchive = io::openZipArchive(stream);
	if (!zipArchive) {
		Log::error("Failed to open zip archive");
		return 0u;
	}
	core::ScopedPtr<io::SeekableReadStream> slicesPng(zipArchive->readStream("slices.png"));
	if (!slicesPng) {
		Log::error("Failed to read slices.png from zip archive");
		return 0u;
	}

	image::ImagePtr image = image::loadImage("slices.png", *slicesPng);
	if (!image || !image->isLoaded()) {
		Log::error("Failed to load slices.png from zip archive");
		return 0u;
	}

	if (!palette.load(image)) {
		Log::error("Failed to load palette from slices.png");
		return 0u;
	}

	return palette.size();
}

} // namespace voxelformat
