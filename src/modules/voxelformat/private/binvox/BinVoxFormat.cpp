/**
 * @file
 */

#include "BinVoxFormat.h"
#include "color/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraphNode.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load binvox file: Not enough data in stream " CORE_STRINGIFY(read));                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
		return false;                                                                                                  \
	}

bool BinVoxFormat::readData(State &state, const core::String &filename, io::SeekableReadStream &stream,
							scenegraph::SceneGraph &sceneGraph) {
	const voxel::Region region(0, 0, 0, (int)state._d - 1, (int)state._w - 1, (int)state._h - 1);
	if (!region.isValid()) {
		Log::error("Invalid region found in file");
		return false;
	}

	voxel::RawVolume *volume = new voxel::RawVolume(region);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(volume, true);
	node.setName(core::string::extractFilename(filename));
	const uint32_t numVoxels = state._w * state._h * state._d;
	uint32_t index = 0;
	uint32_t endIndex = 0;
	const palette::Palette &palette = node.palette();
	while (endIndex < numVoxels) {
		uint8_t value;
		uint8_t count;
		wrap(stream.readUInt8(value))
		// TODO: VOXELFORMAT: there is no official version 3 format spec - but the one file that was sent to
		// me on the vengi discord server that was found, uses either 16 bit palette values or 8 bit palette
		// followed by three unknown bytes. If there ever is a version 3 binvox spec, this might need to be
		// changed
		if (state._version >= 3) {
			if (value == 0u) {
				wrap(stream.readUInt8(value))
			} else {
				stream.skipDelta(1);
			}
			if (value == 0u) {
				wrap(stream.readUInt8(value))
			} else {
				stream.skipDelta(1);
			}
			if (value == 0u) {
				wrap(stream.readUInt8(value))
			} else {
				stream.skipDelta(1);
			}
		}
		wrap(stream.readUInt8(count))
		endIndex = index + count;
		if (endIndex > numVoxels) {
			Log::error("Given count is out of bounds: %i/%i", endIndex, numVoxels);
			return false;
		}
		if (value != 0u) {
			const voxel::Voxel voxel = voxel::createVoxel(palette, value);
			// The y-coordinate runs fastest, then the z-coordinate, then the x-coordinate.
			voxel::RawVolume::Sampler sampler(volume);
			const uint32_t end = endIndex;
			const int32_t w = (int32_t)state._w;
			const int32_t h = (int32_t)state._h;
			uint32_t i = index;
			int32_t ix = (int32_t)(i / (state._w * state._h));
			int32_t iy = (int32_t)(i % state._w);
			int32_t iz = (int32_t)((i / state._w) % state._h);
			sampler.setPosition(ix, iy, iz);
			for (; i < end; ++i) {
				sampler.setVoxel(voxel);
				++iy;
				if (iy >= w) {
					iy = 0;
					++iz;
					if (iz >= h) {
						iz = 0;
						++ix;
					}
					// setPosition is needed when we jump to next row/slice
					sampler.setPosition(ix, iy, iz);
				} else {
					sampler.movePositiveY();
				}
			}
		}
		index = endIndex;
	}
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool BinVoxFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
							  scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	char line[512];

	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	wrapBool(stream->readLine(sizeof(line), line))
	State state;
	if (SDL_sscanf(line, "#binvox %u", &state._version) != 1) {
		Log::error("Failed to parse binvox version");
		return false;
	}
	if (state._version != 1 && state._version != 2) {
		Log::warn("Only version 1 and 2 are supported. Found version %i", state._version);
	}

	for (;;) {
		wrapBool(stream->readLine(sizeof(line), line))
		if (core::string::startsWith(line, "dim ")) {
			if (SDL_sscanf(line, "dim %u %u %u", &state._d, &state._h, &state._w) != 3) {
				Log::error("Failed to parse binvox dimensions");
				return false;
			}
		} else if (core::string::startsWith(line, "translate ")) {
			float tx, ty, tz;
			if (SDL_sscanf(line, "translate %f %f %f", &tz, &ty, &tx) != 3) {
				Log::error("Failed to parse binvox translation");
				return false;
			}
			state._tx = -tx;
			state._ty = -ty;
			state._tz = -tz;
		} else if (core::string::startsWith(line, "scale ")) {
			if (SDL_sscanf(line, "scale %f", &state._scale) != 1) {
				Log::error("Failed to parse binvox scale");
				return false;
			}
		} else if (core::string::startsWith(line, "data")) {
			break;
		} else {
			Log::error("unknown line: '%s'", line);
			return false;
		}
	}
	if (!readData(state, filename, *stream, sceneGraph)) {
		Log::warn("Could not load the data from %s", filename.c_str());
		return false;
	}
	sceneGraph.updateTransforms();
	return true;
}

static bool writeValue(io::SeekableWriteStream *stream, uint8_t value, int binvoxVersion) {
	wrapBool(stream->writeUInt8(value))
	if (binvoxVersion == 3) {
		wrapBool(stream->writeUInt8(0))
		wrapBool(stream->writeUInt8(0))
		wrapBool(stream->writeUInt8(0))
	}
	return true;
}

bool BinVoxFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							  const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	core_assert(node);
	const voxel::Region &region = node->region();
	voxel::RawVolume::Sampler sampler(node->volume());

	const int32_t width = region.getWidthInVoxels();
	const int32_t height = region.getHeightInVoxels();
	const int32_t depth = region.getDepthInVoxels();
	const glm::ivec3 mins = region.getLowerCorner();
	const glm::ivec3 maxs = region.getUpperCorner();
	const glm::ivec3 &offset = -mins;
	const float scale = 1.0f;

	const int binvoxVersion = core::getVar(cfg::VoxformatBinvoxVersion)->intVal();

	stream->writeStringFormat(false, "#binvox %i\n", binvoxVersion);
	stream->writeStringFormat(false, "dim %u %u %u\n", width, depth, height);
	stream->writeStringFormat(false, "translate %i %i %i\n", offset.x, offset.y, offset.z);
	stream->writeStringFormat(false, "scale %f\n", scale);
	wrapBool(stream->writeString("data\n", false))

	uint8_t count = 0u;
	uint8_t value = 0u;
	uint32_t voxels = 0u;
	const int maxIndex = width * height * depth;
	glm::ivec3 pos = mins;
	const uint8_t emptyColorReplacement = node->palette().findReplacement(0);
	Log::debug("found replacement for %s at index %u: %s at index %u",
			   color::print(node->palette().color(0)).c_str(), 0,
			   color::print(node->palette().color(emptyColorReplacement)).c_str(), emptyColorReplacement);
	for (int idx = 0; idx < maxIndex; ++idx) {
		if (!sampler.setPosition(pos)) {
			Log::error("Failed to set position for index %i (%i:%i:%i) (w:%i,h:%i,d:%i)", idx, pos.x, pos.y, pos.z,
					   width, height, depth);
			return false;
		}
		const voxel::Voxel voxel = sampler.voxel();
		if (isAir(voxel.getMaterial())) {
			if (value != 0u || count == 255u) {
				if (count > 0u) {
					wrapBool(writeValue(stream, value, binvoxVersion))
					wrapBool(stream->writeUInt8(count))
				}
				voxels += count;
				count = 0u;
			}
			++count;
			value = 0u;
		} else {
			uint8_t v = voxel.getColor();
			if (v == 0) {
				v = emptyColorReplacement;
			}
			if (binvoxVersion == 1 && v != 0u) {
				// for version 1 we only store a one to indicate a solid voxel
				v = 1u;
			}
			if (value != v || count == 255u) {
				if (count > 0u) {
					wrapBool(writeValue(stream, value, binvoxVersion))
					wrapBool(stream->writeUInt8(count))
				}
				voxels += count;
				count = 0u;
			}
			++count;
			value = v;
		}
		++pos.y;
		if (pos.y > maxs.y) {
			pos.y = mins.y;
			++pos.z;
		}
		if (pos.z > maxs.z) {
			++pos.x;
			pos.y = mins.y;
			pos.z = mins.z;
		}
	}
	core_assert_msg(count > 0u, "Expected to have at least one voxel left: %i", (int)count);
	wrapBool(writeValue(stream, value, binvoxVersion))
	wrapBool(stream->writeUInt8(count))
	voxels += count;
	const uint32_t expectedVoxels = width * height * depth;
	if (voxels != expectedVoxels) {
		Log::error("Not enough data was written: %u vs %u (w: %u, h: %u, d: %u)", voxels, expectedVoxels, width, height,
				   depth);
		return false;
	}
	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
