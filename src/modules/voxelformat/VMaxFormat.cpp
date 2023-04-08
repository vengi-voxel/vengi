/**
 * @file
 */

#include "VMaxFormat.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"
#include "io/LZFSEReadStream.h"
#include "io/ZipArchive.h"
#include "voxel/Palette.h"

namespace voxelformat {

bool VMaxFormat::loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
								   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
								   const LoadContext &ctx) {
	io::ZipArchive archive;
	if (!archive.open(&stream)) {
		Log::error("Failed to open zip archive %s", filename.c_str());
		return false;
	}

	io::BufferedReadWriteStream contentsStream;
	if (!archive.load("contents.vmaxb", contentsStream)) {
		Log::error("Failed to load contents.vmaxb from %s", filename.c_str());
		return false;
	}

	if (contentsStream.seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream for %s", filename.c_str());
		return 0u;
	}
	io::LZFSEReadStream lzfseStream(contentsStream);
	// Apple binary property list
	uint32_t magic;
	lzfseStream.readUInt32(magic);
	if (magic != FourCC('b', 'v', 'x', '2')) {
		return false;
	}

	Log::error("Not yet supported to load the voxel data");

	return false;
}

image::ImagePtr VMaxFormat::loadScreenshot(const core::String &filename, io::SeekableReadStream &stream,
										   const LoadContext &ctx) {
	io::ZipArchive archive;
	if (!archive.open(&stream)) {
		Log::error("Failed to open zip archive %s", filename.c_str());
		return image::ImagePtr();
	}

	io::BufferedReadWriteStream contentsStream;
	if (!archive.load("QuickLook/Thumbnail.png", contentsStream)) {
		Log::error("Failed to load QuickLook/Thumbnail.png from %s", filename.c_str());
		return image::ImagePtr();
	}

	if (contentsStream.seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream for %s", filename.c_str());
		return 0u;
	}
	return image::loadImage("Thumbnail.png", contentsStream);
}

size_t VMaxFormat::loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
							   const LoadContext &ctx) {
	io::ZipArchive archive;
	if (!archive.open(&stream)) {
		Log::error("Failed to open zip archive %s", filename.c_str());
		return 0u;
	}

	io::BufferedReadWriteStream contentsStream;
	if (!archive.load("palette.png", contentsStream)) {
		Log::error("Failed to load palette.png from %s", filename.c_str());
		return 0u;
	}

	if (contentsStream.seek(0) == -1) {
		Log::error("Failed to seek to the beginning of the sub stream for %s", filename.c_str());
		return 0u;
	}

	const image::ImagePtr &img = image::loadImage("palette.png", contentsStream);
	if (!img->isLoaded()) {
		Log::error("Failed to load image palette.png");
		return false;
	}
	if (!palette.load(img)) {
		Log::error("Failed to load palette from image palette.png");
		return false;
	}
	return palette.colorCount();
}

} // namespace voxelformat
