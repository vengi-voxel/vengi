/**
 * @file
 */

#include "AsepriteFormat.h"
#include "app/Async.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "io/Stream.h"
#include "math/Axis.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
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

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#endif
#include "voxelformat/external/cute_aseprite.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

namespace voxelformat {

bool AsepriteFormat::addFrame(scenegraph::SceneGraph &sceneGraph, const core::String &filename, const palette::Palette &palette,
					 const LoadContext &ctx, const ase_t *ase, int frameIndex, math::Axis axis, int offset) const {
	const ase_frame_t *frame = ase->frames + frameIndex;
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	voxel::Region region(0, 0, 0, ase->w - 1, ase->h - 1, 1);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	node.setVolume(v, true);
	node.setName(core::String::format("%s_%d", filename.c_str(), frameIndex));
	node.setPalette(palette);
	palette::PaletteLookup palLookup(palette);
	auto fn = [&palLookup, ase, frame, v, this](int start, int end) {
		voxel::RawVolume::Sampler sampler(v);
		sampler.setPosition(0, ase->h - 1, 0);
		for (int y = 0; y < ase->h; ++y) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int x = 0; x < ase->w; ++x) {
				const ase_color_t pixel = frame->pixels[x + y * ase->w];
				if (pixel.a == 0) {
					sampler2.movePositiveX();
					continue;
				}
				const color::RGBA color = flattenRGB(pixel.r, pixel.g, pixel.b, pixel.a);
				const int index = palLookup.findClosestIndex(color);
				sampler2.setVoxel(voxel::createVoxel(voxel::VoxelType::Generic, index));
				sampler2.movePositiveX();
			}
			sampler.moveNegativeY();
		}
	};
	app::for_parallel(0, ase->w, fn);
	glm::ivec3 sliceOffset(0);
	sliceOffset[math::getIndexForAxis(axis)] = offset;
	sliceOffset *= frameIndex;
	node.volume()->translate(sliceOffset);
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

ase_t *AsepriteFormat::loadAseprite(const core::String &filename, const io::ArchivePtr &archive) const {
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
	const core::String filenameNoPath = core::string::extractFilename(filename);
	const int offset = core::Var::getSafe(cfg::VoxformatImageSliceOffset)->intVal();
	const math::Axis axis = math::toAxis(core::Var::getSafe(cfg::VoxformatImageSliceOffsetAxis)->strVal());
	for (int i = 0; i < ase->frame_count; ++i) {
		if (!addFrame(sceneGraph, filenameNoPath, palette, ctx, ase, i, axis, offset)) {
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
		palette.setColor(i, flattenRGB(color.r, color.g, color.b, color.a));
	}
	cute_aseprite_free(ase);

	return true;
}

} // namespace voxelformat
