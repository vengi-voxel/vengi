/**
 * @file
 */

#include "GoxTxtFormat.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "glm/vector_relational.hpp"
#include "palette/PaletteLookup.h"
#include "palette/RGBABuffer.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MaterialColor.h"
#include "voxel/Voxel.h"
#include "palette/Palette.h"
#include "engine-config.h"
#include <glm/common.hpp>

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load txt file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not load txt file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

bool GoxTxtFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	char buf[64];

	wrapBool(stream->readLine(sizeof(buf), buf))
	if (strncmp(buf, "# Goxel Voxel List", 18) != 0) {
		Log::error("Unexpected magic line: '%s'", buf);
		return false;
	}

	int64_t pos = stream->pos();

	palette::RGBABuffer colors;
	glm::ivec3 mins{0};
	glm::ivec3 maxs{0};
	while (stream->readLine(sizeof(buf), buf)) {
		if (buf[0] == '\n' || buf[0] == '#') {
			continue;
		}
		int x, y, z;
		uint32_t rgb[3];
		if (sscanf(buf, "%i %i %i %02x%02x%02x", &x, &z, &y, &rgb[0], &rgb[1], &rgb[2]) != 6) {
			Log::error("Failed to parse voxel data line");
			return false;
		}
		const core::RGBA rgba(rgb[0], rgb[1], rgb[2], 255);
		colors.put(rgba, true);
		mins = glm::min(mins, glm::ivec3(x, y, z));
		maxs = glm::max(maxs, glm::ivec3(x, y, z));
	}

	core::Buffer<core::RGBA> colorsBuf;
	colorsBuf.reserve(colors.size());
	for (const auto &e : colors) {
		colorsBuf.push_back(e->first);
	}
	colors.clear();
	palette.quantize(colorsBuf.data(), colorsBuf.size());
	palette.markDirty();

	stream->seek(pos, SEEK_SET);
	voxel::Region region(mins, maxs);
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}
	if (glm::any(glm::greaterThan(region.getDimensionsInVoxels(), glm::ivec3(MaxRegionSize)))) {
		Log::warn("Size of matrix exceeds the max allowed value");
		return false;
	}
	if (glm::any(glm::lessThan(region.getDimensionsInVoxels(), glm::ivec3(1)))) {
		Log::warn("Size of matrix results in empty space");
		return false;
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setName(core::string::extractFilename(filename));
	node.setPalette(palette);

	palette::PaletteLookup palLookup(palette);
	while (stream->readLine(sizeof(buf), buf)) {
		if (buf[0] == '\n' || buf[0] == '#') {
			continue;
		}
		int x, y, z;
		uint32_t rgb[3];
		if (sscanf(buf, "%i %i %i %02x%02x%02x", &x, &z, &y, &rgb[0], &rgb[1], &rgb[2]) != 6) {
			Log::error("Failed to parse voxel data line");
			return false;
		}

		const core::RGBA color(rgb[0], rgb[1], rgb[2], 255);
		const uint8_t idx = palLookup.findClosestIndex(color);
		const voxel::Voxel voxel = voxel::createVoxel(palette, idx);
		volume->setVoxel(x, y, z, voxel);
	}
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool GoxTxtFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	stream->writeString("# Goxel - generated with vengi version " PROJECT_VERSION " github.com/vengi-voxel/vengi\n", false);
	stream->writeString("# One line per voxel\n", false);
    stream->writeString("# X Y Z RRGGBB\n", false);

	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);

	const voxel::Region &region = node->volume()->region();
	voxel::RawVolume::Sampler sampler(node->volume());
	const glm::ivec3 &lower = region.getLowerCorner();

	const uint32_t width = region.getWidthInVoxels();
	const uint32_t height = region.getHeightInVoxels();
	const uint32_t depth = region.getDepthInVoxels();
	for (uint32_t x = 0u; x < width; ++x) {
		for (uint32_t y = 0u; y < height; ++y) {
			for (uint32_t z = 0u; z < depth; ++z) {
				core_assert_always(sampler.setPosition(lower.x + x, lower.y + y, lower.z + z));
				const voxel::Voxel &voxel = sampler.voxel();
				if (voxel.getMaterial() == voxel::VoxelType::Air) {
					continue;
				}
				const core::RGBA rgba = node->palette().color(voxel.getColor());
				stream->writeStringFormat(false, "%i %i %i %x%x%x\n", x, z, y, rgba.r, rgba.g, rgba.b);
			}
		}
	}
	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
