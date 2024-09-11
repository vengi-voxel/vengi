/**
 * @file
 */

#include "AsepriteFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/Buffer.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#define CUTE_ASEPRITE_IMPLEMENTATION
#define CUTE_ASEPRITE_ASSERT core_assert
#define CUTE_ASEPRITE_MEMSET core_memset
#define CUTE_ASEPRITE_MEMCPY core_memcpy
#define CUTE_ASEPRITE_ALLOC(size, ctx) core_malloc(size)
#define CUTE_ASEPRITE_FREE(mem, ctx) core_free(mem)
#define CUTE_ASEPRITE_WARNING(msg) Log::warn("%s", msg)
#include "voxelformat/external/cute_aseprite.h"

namespace voxelformat {

static bool addFrame(scenegraph::SceneGraph &sceneGraph, const core::String &filename, const palette::Palette &palette,
					 const LoadContext &ctx, ase_t *ase, int frameIndex) {
	const ase_frame_t *frame = ase->frames + frameIndex;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	voxel::Region region(0, 0, 0, ase->w - 1, ase->h - 1, 1);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	node.setVolume(v, true);
	node.setName(core::string::format("%s_%d", filename.c_str(), frameIndex));
	node.setPalette(palette);
	for (int x = 0; x < ase->w; ++x) {
		for (int y = 0; y < ase->h; ++y) {
			const ase_color_t pixel = frame->pixels[x + y * ase->w];
			if (pixel.a == 0) {
				continue;
			}
			const core::RGBA color(pixel.r, pixel.g, pixel.b, pixel.a);
			const int index = palette.getClosestMatch(color);
			v->setVoxel(x, ase->h - y - 1, 0, voxel::createVoxel(voxel::VoxelType::Generic, index));
		}
	}
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

static ase_t *loadAseprite(const core::String &filename, const io::ArchivePtr &archive) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open file '%s'", filename.c_str());
		return nullptr;
	}
	const size_t size = stream->size();
	core::Buffer<uint8_t> buffer(size);
	if (stream->read(buffer.data(), size) != (int)size) {
		Log::error("Failed to read file '%s'", filename.c_str());
		return nullptr;
	}

	ase_t *ase = cute_aseprite_load_from_memory(buffer.data(), buffer.size(), nullptr);
	if (ase == nullptr) {
		Log::error("Failed to load Aseprite file '%s'", filename.c_str());
		return nullptr;
	}
	return ase;
}

bool AsepriteFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
									scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
									const LoadContext &ctx) {
	ase_t *ase = loadAseprite(filename, archive);
	if (ase == nullptr) {
		return false;
	}
	for (int i = 0; i < ase->frame_count; ++i) {
		if (!addFrame(sceneGraph, filename, palette, ctx, ase, i)) {
			Log::error("Failed to add frame %d from Aseprite file '%s'", i, filename.c_str());
			cute_aseprite_free(ase);
			return false;
		}
	}
	cute_aseprite_free(ase);
	return true;
}

size_t AsepriteFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive,
								   palette::Palette &palette, const LoadContext &ctx) {
	ase_t *ase = loadAseprite(filename, archive);
	if (ase == nullptr) {
		return false;
	}

	ase_palette_t &asePalette = ase->palette;
	palette.setSize(asePalette.entry_count);
	for (int i = 0; i < asePalette.entry_count; ++i) {
		const ase_palette_entry_t &entry = asePalette.entries[i];
		const ase_color_t &color = entry.color;
		palette.setColor(i, core::RGBA(color.r, color.g, color.b, color.a));
	}
	cute_aseprite_free(ase);

	return true;
}

} // namespace voxelformat
