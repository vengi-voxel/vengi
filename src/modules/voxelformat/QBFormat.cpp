/**
 * @file
 */

#include "QBFormat.h"
#include "core/Enum.h"
#include "core/Color.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "io/FileStream.h"
#include "io/Stream.h"
#include "voxel/MaterialColor.h"
#include "voxel/PaletteLookup.h"
#include "voxelformat/SceneGraphNode.h"

namespace voxelformat {

namespace qb {
const int RLE_FLAG = 2;
const int NEXT_SLICE_FLAG = 6;
}

#define wrapSave(write) \
	if ((write) == false) { \
		Log::error("Could not save qb file: " CORE_STRINGIFY(write) " failed"); \
		return false; \
	}

#define wrapSaveColor(color) \
	wrapSave(stream.writeUInt8((color).r)) \
	wrapSave(stream.writeUInt8((color).g)) \
	wrapSave(stream.writeUInt8((color).b)) \
	wrapSave(stream.writeUInt8((color).a))


#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) == false) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return false; \
	}

#define wrapColor(read) \
	if ((read) != 0) { \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read)); \
		return voxel::Voxel(); \
	}

bool QBFormat::saveMatrix(io::SeekableWriteStream& stream, const SceneGraphNode& node) const {
	const int nameLength = (int)node.name().size();
	wrapSave(stream.writeUInt8(nameLength));
	wrapSave(stream.writeString(node.name(), false));

	const voxel::Region& region = node.region();
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}
	const glm::ivec3 size = region.getDimensionsInVoxels();
	wrapSave(stream.writeUInt32(size.x));
	wrapSave(stream.writeUInt32(size.y));
	wrapSave(stream.writeUInt32(size.z));

	const int frame = 0;
	const SceneGraphTransform &transform = node.transform(frame);
	const glm::ivec3 offset = glm::round(transform.worldTranslation());
	wrapSave(stream.writeInt32(offset.x));
	wrapSave(stream.writeInt32(offset.y));
	wrapSave(stream.writeInt32(offset.z));

	constexpr voxel::Voxel Empty;

	const glm::ivec3& mins = region.getLowerCorner();
	const glm::ivec3& maxs = region.getUpperCorner();

	core::RGBA currentColor;
	uint32_t count = 0;

	const voxel::Palette& palette = node.palette();

	for (int z = mins.z; z <= maxs.z; ++z) {
		for (int y = mins.y; y <= maxs.y; ++y) {
			for (int x = mins.x; x <= maxs.x; ++x) {
				const voxel::Voxel& voxel = node.volume()->voxel(x, y, z);
				core::RGBA newColor;
				if (voxel == Empty) {
					newColor = 0;
					Log::trace("Save empty voxel: x %i, y %i, z %i", x, y, z);
				} else {
					newColor = palette.colors[voxel.getColor()];
					Log::trace("Save voxel: x %i, y %i, z %i (color: index(%i) => rgba(%i:%i:%i:%i))",
							x, y, z, (int)voxel.getColor(), (int)newColor.r, (int)newColor.g, (int)newColor.b, (int)newColor.a);
				}

				if (newColor != currentColor) {
					if (count == 1) {
						wrapSaveColor(currentColor)
					} else if (count == 2) {
						wrapSaveColor(currentColor)
						wrapSaveColor(currentColor)
					} else if (count == 3) {
						wrapSaveColor(currentColor)
						wrapSaveColor(currentColor)
						wrapSaveColor(currentColor)
					} else if (count > 3) {
						wrapSave(stream.writeUInt32(qb::RLE_FLAG))
						wrapSave(stream.writeUInt32(count))
						wrapSaveColor(currentColor)
					}
					count = 0;
					currentColor = newColor;
				}
				count++;
			}
		}
		if (count == 1) {
			wrapSaveColor(currentColor)
		} else if (count == 2) {
			wrapSaveColor(currentColor)
			wrapSaveColor(currentColor)
		} else if (count == 3) {
			wrapSaveColor(currentColor)
			wrapSaveColor(currentColor)
			wrapSaveColor(currentColor)
		} else if (count > 3) {
			wrapSave(stream.writeUInt32(qb::RLE_FLAG))
			wrapSave(stream.writeUInt32(count))
			wrapSaveColor(currentColor)
		}
		count = 0;
		wrapSave(stream.writeUInt32(qb::NEXT_SLICE_FLAG));
	}
	return true;
}

bool QBFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapSave(stream.writeUInt32(131331)) // version
	wrapSave(stream.writeUInt32((uint32_t)ColorFormat::RGBA))
	wrapSave(stream.writeUInt32((uint32_t)ZAxisOrientation::LeftHanded))
	wrapSave(stream.writeUInt32((uint32_t)Compression::RLE))
	wrapSave(stream.writeUInt32((uint32_t)VisibilityMask::AlphaChannelVisibleByValue))
	wrapSave(stream.writeUInt32((uint32_t)sceneGraph.size()))
	for (const SceneGraphNode& node : sceneGraph) {
		if (!saveMatrix(stream, node)) {
			return false;
		}
	}
	return true;
}

voxel::Voxel QBFormat::getVoxel(State& state, io::SeekableReadStream& stream, voxel::PaletteLookup &palLookup) {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
	if (state._colorFormat == ColorFormat::RGBA) {
		wrapColor(stream.readUInt8(red))
		wrapColor(stream.readUInt8(green))
		wrapColor(stream.readUInt8(blue))
	} else {
		wrapColor(stream.readUInt8(blue))
		wrapColor(stream.readUInt8(green))
		wrapColor(stream.readUInt8(red))
	}
	wrapColor(stream.readUInt8(alpha))
	if (alpha == 0) {
		return voxel::Voxel();
	}
	const core::RGBA color = core::Color::getRGBA(red, green, blue, alpha);
	const uint8_t index = palLookup.findClosestIndex(color);
	voxel::Voxel v = voxel::createVoxel(voxel::VoxelType::Generic, index);
	return v;
}

bool QBFormat::loadMatrix(State& state, io::SeekableReadStream& stream, SceneGraph& sceneGraph, voxel::PaletteLookup &palLookup) {
	char name[260] = "";
	uint8_t nameLength;
	wrap(stream.readUInt8(nameLength));
	Log::debug("Matrix name length: %u", (uint32_t)nameLength);
	wrapBool(stream.readString(nameLength, name));
	name[nameLength] = '\0';
	Log::debug("Matrix name: %s", name);

	glm::uvec3 size(0);
	wrap(stream.readUInt32(size.x));
	wrap(stream.readUInt32(size.y));
	wrap(stream.readUInt32(size.z));
	Log::debug("Matrix size: %i:%i:%i", size.x, size.y, size.z);

	if (size.x == 0 || size.y == 0 || size.z == 0) {
		Log::error("Invalid size (%i:%i:%i)", size.x, size.y, size.z);
		return false;
	}

	if (size.x > 2048 || size.y > 2048 || size.z > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", size.x, size.y, size.z);
		return false;
	}

	SceneGraphTransform transform;
	{
		glm::ivec3 offset(0);
		wrap(stream.readInt32(offset.x));
		wrap(stream.readInt32(offset.y));
		wrap(stream.readInt32(offset.z));
		Log::debug("Matrix offset: %i:%i:%i", offset.x, offset.y, offset.z);
		if (state._zAxisOrientation == ZAxisOrientation::RightHanded) {
			core::exchange(offset.x, offset.z);
		}
		transform.setWorldTranslation(offset);
	}

	voxel::Region region;
	if (state._zAxisOrientation == ZAxisOrientation::RightHanded) {
		region = voxel::Region(0, 0, 0, (int)size.z - 1, (int)size.y - 1, (int)size.x - 1);
	} else {
		region = voxel::Region(0, 0, 0, (int)size.x - 1, (int)size.y - 1, (int)size.z - 1);
	}
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}

	if (region.getDepthInVoxels() >= 2048 || region.getHeightInVoxels() >= 2048
		|| region.getWidthInVoxels() >= 2048) {
		Log::error("Region exceeds the max allowed boundaries");
		return false;
	}

	core::ScopedPtr<voxel::RawVolume> v(new voxel::RawVolume(region));
	if (state._compressed == Compression::None) {
		Log::debug("qb matrix uncompressed");
		for (uint32_t z = 0; z < size.z; ++z) {
			for (uint32_t y = 0; y < size.y; ++y) {
				for (uint32_t x = 0; x < size.x; ++x) {
					const voxel::Voxel& voxel = getVoxel(state, stream, palLookup);
					if (state._zAxisOrientation == ZAxisOrientation::RightHanded) {
						v->setVoxel((int)z, (int)y, (int)x, voxel);
					} else {
						v->setVoxel((int)x, (int)y, (int)z, voxel);
					}
				}
			}
		}
		SceneGraphNode node(SceneGraphNodeType::Model);
		node.setVolume(v.release(), true);
		node.setName(name);
		const KeyFrameIndex keyFrameIdx = 0;
		node.setTransform(keyFrameIdx, transform);
		node.setPalette(palLookup.palette());
		sceneGraph.emplace(core::move(node));
		return true;
	}

	Log::debug("Matrix rle compressed");

	uint32_t z = 0u;
	while (z < size.z) {
		uint32_t index = 0;
		for (;;) {
			uint32_t data;
			wrap(stream.peekUInt32(data))
			if (data == qb::NEXT_SLICE_FLAG) {
				stream.skip(sizeof(data));
				break;
			}

			uint32_t count = 1;
			if (data == qb::RLE_FLAG) {
				stream.skip(sizeof(data));
				wrap(stream.readUInt32(count))
				Log::trace("%u voxels of the same type", count);
			}

			if (count > size.x * size.y * size.z) {
				Log::error("Max RLE count exceeded: %u (%u:%u:%u)", count, size.x, size.y, size.z);
				return false;
			}
			const voxel::Voxel &voxel = getVoxel(state, stream, palLookup);
			for (uint32_t j = 0; j < count; ++j) {
				const uint32_t x = (index + j) % size.x;
				const uint32_t y = (index + j) / size.x;
				if (state._zAxisOrientation == ZAxisOrientation::RightHanded) {
					v->setVoxel((int)z, (int)y, (int)x, voxel);
				} else {
					v->setVoxel((int)x, (int)y, (int)z, voxel);
				}
			}
			index += count;
		}
		++z;
	}
	SceneGraphNode node(SceneGraphNodeType::Model);
	node.setVolume(v.release(), true);
	node.setName(name);
	node.setPalette(palLookup.palette());
	const KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	sceneGraph.emplace(core::move(node));
	Log::debug("Matrix read");
	return true;
}

bool QBFormat::loadGroups(const core::String& filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	State state;
	wrap(stream.readUInt32(state._version))
	uint32_t colorFormat;
	wrap(stream.readUInt32(colorFormat))
	state._colorFormat = (ColorFormat)colorFormat;
	uint32_t zAxisOrientation;
	wrap(stream.readUInt32(zAxisOrientation))
	state._zAxisOrientation = (ZAxisOrientation)zAxisOrientation;
	uint32_t compressed;
	wrap(stream.readUInt32(compressed))
	state._compressed = (Compression)compressed;
	uint32_t visibilityMaskEncoded;
	wrap(stream.readUInt32(visibilityMaskEncoded))
	state._visibilityMaskEncoded = (VisibilityMask)visibilityMaskEncoded;

	uint32_t numMatrices;
	wrap(stream.readUInt32(numMatrices))
	if (numMatrices > 16384) {
		Log::error("Max allowed matrices exceeded: %u", numMatrices);
		return false;
	}

	Log::debug("Version: %u", state._version);
	Log::debug("ColorFormat: %u", core::enumVal(state._colorFormat));
	Log::debug("ZAxisOrientation: %u", core::enumVal(state._zAxisOrientation));
	Log::debug("Compressed: %u", core::enumVal(state._compressed));
	Log::debug("VisibilityMaskEncoded: %u", core::enumVal(state._visibilityMaskEncoded));
	Log::debug("NumMatrices: %u", numMatrices);

	sceneGraph.reserve(numMatrices);
	voxel::PaletteLookup palLookup;
	for (uint32_t i = 0; i < numMatrices; i++) {
		Log::debug("Loading matrix: %u", i);
		if (!loadMatrix(state, stream, sceneGraph, palLookup)) {
			Log::error("Failed to load the matrix %u", i);
			break;
		}
	}
	sceneGraph.updateTransforms();
	return true;
}

}

#undef wrap
#undef wrapBool
#undef wrapSave
#undef wrapSaveColor
#undef wrapColor
