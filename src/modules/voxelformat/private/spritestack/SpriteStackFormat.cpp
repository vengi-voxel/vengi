/**
 * @file
 */

#include "SpriteStackFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"
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

	// Try to read slices.json first, then fallback to spritesheet.json
	core::ScopedPtr<io::SeekableReadStream> jsonStream(zipArchive->readStream("slices.json"));
	bool isSpritesheetFormat = false;
	if (!jsonStream) {
		jsonStream = zipArchive->readStream("spritesheet.json");
		if (!jsonStream) {
			Log::error("Failed to read slices.json or spritesheet.json from zip archive");
			return false;
		}
		isSpritesheetFormat = true;
	}
	core::String jsonString;
	jsonStream->readString(jsonStream->remaining(), jsonString);
	nlohmann::json json = nlohmann::json::parse(jsonString, nullptr, false, true);
	if (json.is_discarded()) {
		Log::error("Failed to parse JSON: %s", jsonString.c_str());
		return false;
	}

	struct SlicesInfo {
		int slices = 0;
		int frames = 1; // frames per row in the atlas (default 1)
		int width = 0;
		int height = 0;
		int angles = 0;					 // for spritesheet format
		core::DynamicArray<int> regions; // for optimized format
		core::DynamicArray<int> trims;	 // for optimized format
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
	if (json.contains("angles") && json["angles"].is_number_integer()) {
		info.angles = json["angles"].get<int>();
		isSpritesheetFormat = true;
		// For spritesheet format, angles defines the number of slices
		if (info.slices == 0) {
			info.slices = info.angles;
		}
	}

	// Handle optimized spritesheet format with regions and trims
	if (json.contains("regions") && json["regions"].is_array()) {
		const auto &regions = json["regions"];
		for (const auto &val : regions) {
			if (val.is_number_integer()) {
				info.regions.push_back(val.get<int>());
			}
		}
	}
	if (json.contains("trims") && json["trims"].is_array()) {
		const auto &trims = json["trims"];
		for (const auto &val : trims) {
			if (val.is_number_integer()) {
				info.trims.push_back(val.get<int>());
			}
		}
	}

	if (info.slices <= 0 || info.frames <= 0 || info.width <= 0 || info.height <= 0) {
		Log::error("Invalid data: %d slices, %d frames, %d width, %d height", info.slices, info.frames, info.width,
				   info.height);
		return false;
	}

	const char *imageName = isSpritesheetFormat ? "spritesheet.png" : "slices.png";
	core::ScopedPtr<io::SeekableReadStream> imageStream(zipArchive->readStream(imageName));
	if (!imageStream) {
		Log::error("Failed to read %s from zip archive", imageName);
		return false;
	}

	image::ImagePtr image = image::loadImage(imageName, *imageStream);
	if (!image || !image->isLoaded()) {
		Log::error("Failed to load %s from zip archive", imageName);
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

	// Check if this is an optimized spritesheet with regions
	const bool hasRegions = !info.regions.empty() && info.regions.size() >= (size_t)info.slices * 4;

	const bool matchesAtlas = (atlasCols * info.width == imgW) && (atlasRows * info.height == imgH);
	const bool matchesVerticalStack = (info.width == imgW) && (info.height * info.slices == imgH);
	const bool matchesHorizontalStack = (info.height == imgH) && (info.width * info.slices == imgW);
	const bool matchesSingle = (info.width == imgW) && (info.height == imgH) && info.slices == 1;

	if (!hasRegions && !matchesAtlas && !matchesVerticalStack && !matchesHorizontalStack && !matchesSingle) {
		Log::debug("%s size %dx%d doesn't match expected atlas (%dx%d) or stacks. Trying best-effort extraction.",
				   imageName, imgW, imgH, atlasCols * info.width, atlasRows * info.height);
		return false;
	}

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(core::String::format("%s_model_%s", isSpritesheetFormat ? "spritesheet" : "slices", filename.c_str()));

	voxel::Region region(0, 0, 0, info.width - 1, info.height - 1, info.slices - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	palette::PaletteLookup palLookup(palette);
	node.setVolume(volume, true);
	node.setPalette(palette);
	Log::debug("Region width: %d, height: %d, depth: %d", region.getWidthInVoxels(), region.getHeightInVoxels(),
			   region.getDepthInVoxels());
	Log::debug("Slices: %d, Frames: %d, Width: %d, Height: %d, Angles: %d", info.slices, info.frames, info.width,
			   info.height, info.angles);
	Log::debug("atlasCols: %d, atlasRows: %d, imgW: %d, imgH: %d", atlasCols, atlasRows, imgW, imgH);

	// For each slice i, compute sub-image origin in the atlas and iterate pixels:
	for (int i = 0; i < info.slices; ++i) {
		int sx, sy, sw, sh;
		int trimLeft = 0, trimTop = 0;

		if (hasRegions) {
			// Optimized format with regions [x, y, w, h] for each slice
			const int idx = i * 4;
			sx = info.regions[idx];
			sy = info.regions[idx + 1];
			sw = info.regions[idx + 2];
			sh = info.regions[idx + 3];

			// Apply trims if available (2 values per slice: left trim, top trim)
			if (!info.trims.empty() && (size_t)i * 2 + 1 < info.trims.size()) {
				trimLeft = info.trims[i * 2];
				trimTop = info.trims[i * 2 + 1];
			}
		} else if (matchesVerticalStack) {
			sx = 0;
			sy = i * info.height;
			sw = info.width;
			sh = info.height;
		} else if (matchesHorizontalStack) {
			sx = i * info.width;
			sy = 0;
			sw = info.width;
			sh = info.height;
		} else {
			const int col = i % atlasCols;
			const int row = i / atlasCols;
			sx = col * info.width;
			sy = row * info.height;
			sw = info.width;
			sh = info.height;
		}

		if (sx < 0 || sy < 0 || sx + sw > imgW || sy + sh > imgH) {
			Log::error("Slice %d out of bounds in %s (sx=%d, sy=%d, w=%d, h=%d, img=%dx%d)", i, imageName, sx, sy, sw,
					   sh, imgW, imgH);
			return false;
		}

		for (int yy = 0; yy < sh; ++yy) {
			for (int xx = 0; xx < sw; ++xx) {
				const int px = sx + xx;
				const int py = sy + yy;

				const color::RGBA color = image->colorAt(px, py);
				// skip fully transparent pixels
				if (color.a == 0) {
					continue;
				}

				const uint8_t palIdx = palLookup.findClosestIndex(color);
				const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, palIdx);

				// Apply trim offsets and bounds checking
				const int vx = xx + trimLeft;
				const int vy = sh - 1 - yy + trimTop;
				if (vx >= 0 && vx < info.width && vy >= 0 && vy < info.height) {
					volume->setVoxel(vx, info.height - 1 - vy, i, voxel);
				}
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

	// Try slices.png first, then spritesheet.png
	core::ScopedPtr<io::SeekableReadStream> imageStream(zipArchive->readStream("slices.png"));
	const char *imageName = "slices.png";
	if (!imageStream) {
		imageStream = zipArchive->readStream("spritesheet.png");
		imageName = "spritesheet.png";
		if (!imageStream) {
			Log::error("Failed to read slices.png or spritesheet.png from zip archive");
			return 0u;
		}
	}

	image::ImagePtr image = image::loadImage(imageName, *imageStream);
	if (!image || !image->isLoaded()) {
		Log::error("Failed to load %s from zip archive", imageName);
		return 0u;
	}

	if (!palette.load(image)) {
		Log::error("Failed to load palette from %s", imageName);
		return 0u;
	}

	return palette.size();
}

bool SpriteStackFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								   const io::ArchivePtr &archive, const SaveContext &ctx) {
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	if (node == nullptr) {
		Log::error("No model node found in scene graph");
		return false;
	}

	const voxel::RawVolume *volume = node->volume();
	if (volume == nullptr) {
		Log::error("No volume found in model node");
		return false;
	}

	const palette::Palette &palette = node->palette();
	const voxel::Region &region = volume->region();
	const int width = region.getWidthInVoxels();
	const int height = region.getHeightInVoxels();
	const int slices = region.getDepthInVoxels();

	io::BufferedReadWriteStream zipFileStream(64000);
	{
		io::ZipArchive zipArchive;
		if (!zipArchive.init(&zipFileStream)) {
			Log::error("Failed to initialize ZIP archive for writing");
			return false;
		}

		nlohmann::json json;
		json["slices"] = slices;
		json["frames"] = 1;
		json["width"] = width;
		json["height"] = height;

		const core::String jsonString(json.dump(2).c_str());
		{
			core::ScopedPtr<io::SeekableWriteStream> jsonStream(zipArchive.writeStream("slices.json"));
			if (jsonStream == nullptr) {
				Log::error("Failed to create slices.json in ZIP archive");
				return false;
			}
			if (!jsonStream->writeString(jsonString, false)) {
				Log::error("Failed to write slices.json");
				return false;
			}
		}

		const int imgWidth = width;
		const int imgHeight = height * slices;
		image::ImagePtr image = image::createEmptyImage("slices");
		if (!image->resize(imgWidth, imgHeight)) {
			Log::error("Failed to create image buffer");
			return false;
		}

		// Render each slice into the image
		for (int z = 0; z < slices; ++z) {
			const int yOffset = z * height;
			for (int y = 0; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					const voxel::Voxel &voxel = volume->voxel(x, y, z);
					color::RGBA color;
					if (voxel::isAir(voxel.getMaterial())) {
						color = color::RGBA(0, 0, 0, 0);
					} else {
						color = palette.color(voxel.getColor());
					}
					// Flip Y coordinate for image (images are top-down)
					image->setColor(color::RGBA(color), x, yOffset + (height - 1 - y));
				}
			}
		}

		{
			core::ScopedPtr<io::SeekableWriteStream> imageStream(zipArchive.writeStream("slices.png"));
			if (imageStream == nullptr) {
				Log::error("Failed to create slices.png in ZIP archive");
				return false;
			}

			if (!image->writePNG(*imageStream)) {
				Log::error("Failed to write PNG data");
				return false;
			}
		}

		zipArchive.shutdown();
	}

	core::ScopedPtr<io::SeekableWriteStream> outStream(archive->writeStream(filename));
	if (!outStream) {
		Log::error("Failed to create output stream for %s", filename.c_str());
		return false;
	}

	zipFileStream.seek(0);
	outStream->writeStream(zipFileStream);

	Log::debug("Saved sprite stack to %s (%d slices, %dx%d)", filename.c_str(), slices, width, height);
	return true;
}

} // namespace voxelformat
