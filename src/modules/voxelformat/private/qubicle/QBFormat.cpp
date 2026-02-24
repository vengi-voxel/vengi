/**
 * @file
 */

#include "QBFormat.h"
#include "color/Color.h"
#include "core/Enum.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "core/collection/DynamicMap.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MaterialColor.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxelformat/Format.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

namespace qb {
const int RLE_FLAG = 2;
const int NEXT_SLICE_FLAG = 6;
} // namespace qb

#define wrapSave(write)                                                                                                \
	if ((write) == false) {                                                                                            \
		Log::error("Could not save qb file: " CORE_STRINGIFY(write) " failed");                                        \
		return false;                                                                                                  \
	}

#define wrapSaveWriter(write)                                                                                          \
	if ((write) == false) {                                                                                            \
		Log::error("Could not save qb file: " CORE_STRINGIFY(write) " failed");                                        \
		_error = true;                                                                                                 \
		return;                                                                                                        \
	}

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read));                         \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Could not load qb file: Not enough data in stream " CORE_STRINGIFY(read));                         \
		return false;                                                                                                  \
	}

class MatrixWriter {
private:
	io::SeekableWriteStream &_stream;
	const voxel::RawVolume *_volume;
	const palette::Palette &_palette;
	const glm::ivec3 _maxs;

	const bool _leftHanded;
	const bool _rleCompressed;
	bool _error = false;
	color::RGBA _currentColor;
	uint32_t _count = 0;

	bool saveColor(io::WriteStream &stream, color::RGBA color) {
		// VisibilityMask::AlphaChannelVisibleByValue
		wrapSave(stream.writeUInt8(color.r))
		wrapSave(stream.writeUInt8(color.g))
		wrapSave(stream.writeUInt8(color.b))
		wrapSave(stream.writeUInt8(color.a > 0 ? 255 : 0))
		return true;
	}

public:
	MatrixWriter(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
				 const scenegraph::SceneGraphNode &node, bool leftHanded, bool rleCompressed)
		: _stream(stream), _volume(sceneGraph.resolveVolume(node)), _palette(node.palette()),
		  _maxs(_volume->region().getUpperCorner()), _leftHanded(leftHanded), _rleCompressed(rleCompressed) {
	}

	void addVoxel(int x, int y, int z, const voxel::Voxel &voxel) {
		if (_error) {
			return;
		}
		if (!_rleCompressed) {
			color::RGBA color(0, 0, 0, 0);
			if (voxel::isAir(voxel.getMaterial())) {
				wrapSaveWriter(saveColor(_stream, color))
			} else {
				color = _palette.color(voxel.getColor());
			}
			wrapSaveWriter(saveColor(_stream, color))
			return;
		}
		constexpr voxel::Voxel Empty;
		color::RGBA newColor;
		if (voxel.isSameType(Empty)) {
			newColor = 0u;
			Log::trace("Save empty voxel: x %i, y %i, z %i", x, y, z);
		} else {
			newColor = _palette.color(voxel.getColor());
			Log::trace("Save voxel: x %i, y %i, z %i (color: index(%i) => rgba(%i:%i:%i:%i))", x, y, z,
					   (int)voxel.getColor(), (int)newColor.r, (int)newColor.g, (int)newColor.b, (int)newColor.a);
		}

		if (newColor != _currentColor) {
			if (_count > 3) {
				wrapSaveWriter(_stream.writeUInt32(qb::RLE_FLAG))
				wrapSaveWriter(_stream.writeUInt32(_count))
				wrapSaveWriter(saveColor(_stream, _currentColor))
			} else {
				for (uint32_t i = 0; i < _count; ++i) {
					wrapSaveWriter(saveColor(_stream, _currentColor))
				}
			}
			_count = 0;
			_currentColor = newColor;
		}
		_count++;
		if (y == _maxs.y) {
			bool nextSlice;
			if (_leftHanded) {
				nextSlice = x == _maxs.x;
			} else {
				nextSlice = z == _maxs.z;
			}
			if (nextSlice) {
				if (_count > 3) {
					wrapSaveWriter(_stream.writeUInt32(qb::RLE_FLAG))
					wrapSaveWriter(_stream.writeUInt32(_count))
					wrapSaveWriter(saveColor(_stream, _currentColor))
				} else {
					for (uint32_t i = 0; i < _count; ++i) {
						wrapSaveWriter(saveColor(_stream, _currentColor))
					}
				}
				_count = 0;
				wrapSaveWriter(_stream.writeUInt32(qb::NEXT_SLICE_FLAG));
			}
		}
	}

	bool success() const {
		return !_error;
	}
};

bool QBFormat::saveMatrix(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
						  const scenegraph::SceneGraphNode &node, bool leftHanded, bool rleCompressed) const {
	const int nameLength = (int)node.name().size();
	wrapSave(stream.writeUInt8(nameLength));
	wrapSave(stream.writeString(node.name(), false));

	const voxel::Region &region = sceneGraph.resolveRegion(node);
	if (!region.isValid()) {
		Log::error("Invalid region");
		return false;
	}
	const glm::ivec3 &size = region.getDimensionsInVoxels();
	if (leftHanded) {
		wrapSave(stream.writeUInt32(size.x));
		wrapSave(stream.writeUInt32(size.y));
		wrapSave(stream.writeUInt32(size.z));
	} else {
		wrapSave(stream.writeUInt32(size.z));
		wrapSave(stream.writeUInt32(size.y));
		wrapSave(stream.writeUInt32(size.x));
	}

	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);
	const glm::ivec3 &offset = glm::round(transform.worldTranslation());
	if (leftHanded) {
		wrapSave(stream.writeInt32(offset.x));
		wrapSave(stream.writeInt32(offset.y));
		wrapSave(stream.writeInt32(offset.z));
	} else {
		wrapSave(stream.writeInt32(offset.z));
		wrapSave(stream.writeInt32(offset.y));
		wrapSave(stream.writeInt32(offset.x));
	}
	voxelutil::VisitorOrder visitOrder = voxelutil::VisitorOrder::ZYX;
	if (!leftHanded) {
		visitOrder = voxelutil::VisitorOrder::XYZ;
	}
	MatrixWriter writer(stream, sceneGraph, node, leftHanded, rleCompressed);
	auto func = [&writer](int x, int y, int z, const voxel::Voxel &voxel) { writer.addVoxel(x, y, z, voxel); };
	voxelutil::visitVolume(*sceneGraph.resolveVolume(node), func, voxelutil::VisitAll(), visitOrder);
	return writer.success();
}

bool QBFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						  const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	wrapSave(stream->writeUInt32(257)) // version
	wrapSave(stream->writeUInt32((uint32_t)ColorFormat::RGBA))
	const bool leftHanded = core::Var::getVar(cfg::VoxformatQBSaveLeftHanded)->boolVal();
	const ZAxisOrientation orientation = leftHanded ? ZAxisOrientation::LeftHanded : ZAxisOrientation::RightHanded;
	const bool rleCompressed = core::Var::getVar(cfg::VoxformatQBSaveCompressed)->boolVal();
	wrapSave(stream->writeUInt32((uint32_t)orientation))
	wrapSave(stream->writeUInt32(rleCompressed ? (uint32_t)Compression::RLE : (uint32_t)Compression::None))
	wrapSave(stream->writeUInt32((uint32_t)VisibilityMask::AlphaChannelVisibleByValue))
	wrapSave(stream->writeUInt32((uint32_t)sceneGraph.size(scenegraph::SceneGraphNodeType::AllModels)))
	for (auto iter = sceneGraph.beginAllModels(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &node = *iter;
		if (!saveMatrix(*stream, sceneGraph, node, leftHanded, rleCompressed)) {
			return false;
		}
	}
	return true;
}

voxel::Voxel QBFormat::getVoxel(State &state, io::SeekableReadStream &stream, palette::PaletteLookup &palLookup) {
	color::RGBA color(0);
	if (!readColor(state, stream, color)) {
		return voxel::Voxel();
	}
	if (color.a == 0) {
		return voxel::Voxel();
	}
	const uint8_t index = palLookup.findClosestIndex(flattenRGB(color.r, color.g, color.b));
	voxel::Voxel v = voxel::createVoxel(palLookup.palette(), index);
	return v;
}

bool QBFormat::readColor(State &state, io::SeekableReadStream &stream, color::RGBA &color) {
	if (state._colorFormat == ColorFormat::RGBA) {
		wrap(stream.readUInt8(color.r))
		wrap(stream.readUInt8(color.g))
		wrap(stream.readUInt8(color.b))
	} else {
		wrap(stream.readUInt8(color.b))
		wrap(stream.readUInt8(color.g))
		wrap(stream.readUInt8(color.r))
	}
	// the returned alpha value might also be the vis mask
	// if (mask == 0) // voxel invisble
	// if (mask && 2 == 2) // left side visible
	// if (mask && 4 == 4) // right side visible
	// if (mask && 8 == 8) // top side visible
	// if (mask && 16 == 16) // bottom side visible
	// if (mask && 32 == 32) // front side visible
	// if (mask && 64 == 64) // back side visible
	wrap(stream.readUInt8(color.a))
	return true;
}

bool QBFormat::readMatrix(State &state, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
						  palette::PaletteLookup &palLookup) {
	core::String name;
	wrapBool(stream.readPascalStringUInt8(name))
	Log::debug("Matrix name: %s", name.c_str());

	glm::uvec3 size(0);
	wrap(stream.readUInt32(size.x))
	wrap(stream.readUInt32(size.y))
	wrap(stream.readUInt32(size.z))
	Log::debug("Matrix size: %i:%i:%i", size.x, size.y, size.z);

	if (size.x == 0 || size.y == 0 || size.z == 0) {
		Log::error("Invalid size (%i:%i:%i)", size.x, size.y, size.z);
		return false;
	}

	if (size.x > 2048 || size.y > 2048 || size.z > 2048) {
		Log::error("Volume exceeds the max allowed size: %i:%i:%i", size.x, size.y, size.z);
		return false;
	}

	scenegraph::SceneGraphTransform transform;
	{
		glm::ivec3 offset(0);
		if (state._zAxisOrientation == ZAxisOrientation::LeftHanded) {
			wrap(stream.readInt32(offset.x))
			wrap(stream.readInt32(offset.y))
			wrap(stream.readInt32(offset.z))
		} else {
			wrap(stream.readInt32(offset.z))
			wrap(stream.readInt32(offset.y))
			wrap(stream.readInt32(offset.x))
		}
		Log::debug("Matrix offset: %i:%i:%i", offset.x, offset.y, offset.z);
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

	if (region.getDepthInVoxels() >= 2048 || region.getHeightInVoxels() >= 2048 || region.getWidthInVoxels() >= 2048) {
		Log::error("Region exceeds the max allowed boundaries");
		return false;
	}

	core::ScopedPtr<voxel::RawVolume> v(new voxel::RawVolume(region));
	if (state._compressed == Compression::None) {
		Log::debug("qb matrix uncompressed");
		voxel::RawVolume::Sampler sampler(*v);
		sampler.setPosition(0, 0, 0);
		if (state._zAxisOrientation == ZAxisOrientation::LeftHanded) {
			for (uint32_t z = 0; z < size.z; ++z) {
				voxel::RawVolume::Sampler sampler2 = sampler;
				for (uint32_t y = 0; y < size.y; ++y) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (uint32_t x = 0; x < size.x; ++x) {
						const voxel::Voxel &voxel = getVoxel(state, stream, palLookup);
						sampler3.setVoxel(voxel);
						sampler3.movePositiveX();
					}
					sampler2.movePositiveY();
				}
				sampler.movePositiveZ();
			}
		} else {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (uint32_t x = 0; x < size.x; ++x) {
				for (uint32_t y = 0; y < size.y; ++y) {
					voxel::RawVolume::Sampler sampler3 = sampler2;
					for (uint32_t z = 0; z < size.z; ++z) {
						const voxel::Voxel &voxel = getVoxel(state, stream, palLookup);
						sampler3.setVoxel(voxel);
						sampler3.movePositiveZ();
					}
					sampler2.movePositiveY();
				}
				sampler.movePositiveX();
			}
		}
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(v.release(), true);
		node.setName(name);
		const scenegraph::KeyFrameIndex keyFrameIdx = 0;
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
			// TODO: PERF: use a sampler
			if (state._zAxisOrientation == ZAxisOrientation::RightHanded) {
				for (uint32_t j = 0; j < count; ++j) {
					const uint32_t x = (index + j) % size.x;
					const uint32_t y = (index + j) / size.x;
					v->setVoxel((int)z, (int)y, (int)x, voxel);
				}
			} else {
				for (uint32_t j = 0; j < count; ++j) {
					const uint32_t x = (index + j) % size.x;
					const uint32_t y = (index + j) / size.x;
					v->setVoxel((int)x, (int)y, (int)z, voxel);
				}
			}
			index += count;
		}
		++z;
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(v.release(), true);
	node.setName(name);
	node.setPalette(palLookup.palette());
	const scenegraph::KeyFrameIndex keyFrameIdx = 0;
	node.setTransform(keyFrameIdx, transform);
	sceneGraph.emplace(core::move(node));
	Log::debug("Matrix read");
	return true;
}

bool QBFormat::readPalette(State &state, io::SeekableReadStream &stream, palette::RGBABuffer &colors) {
	uint8_t nameLength;
	wrap(stream.readUInt8(nameLength));
	if (stream.skip(nameLength) == -1) {
		Log::error("Failed to skip name bytes");
		return false;
	}

	glm::uvec3 size(0);
	wrap(stream.readUInt32(size.x));
	wrap(stream.readUInt32(size.y));
	wrap(stream.readUInt32(size.z));
	Log::debug("Matrix size: %i:%i:%i", size.x, size.y, size.z);

	if (size.x == 0 || size.y == 0 || size.z == 0) {
		Log::error("Invalid size (%u:%u:%u)", size.x, size.y, size.z);
		return false;
	}

	if (size.x > 2048 || size.y > 2048 || size.z > 2048) {
		Log::error("Volume exceeds the max allowed size: %u:%u:%u", size.x, size.y, size.z);
		return false;
	}

	int32_t tmp;
	wrap(stream.readInt32(tmp));
	wrap(stream.readInt32(tmp));
	wrap(stream.readInt32(tmp));

	if (state._compressed == Compression::None) {
		Log::debug("qb matrix uncompressed");
		// TODO: PERF: use a sampler
		for (uint32_t z = 0; z < size.z; ++z) {
			for (uint32_t y = 0; y < size.y; ++y) {
				for (uint32_t x = 0; x < size.x; ++x) {
					color::RGBA color(0);
					wrapBool(readColor(state, stream, color))
					if (color.a == 0) {
						continue;
					}
					colors.put(flattenRGB(color.r, color.g, color.b), true);
				}
			}
		}
	} else {
		Log::debug("qb matrix rle compressed");
		uint32_t z = 0u;
		while (z < size.z) {
			for (;;) {
				uint32_t data;
				wrap(stream.peekUInt32(data))
				if (data == qb::NEXT_SLICE_FLAG) {
					stream.skip(sizeof(data));
					break;
				}
				if (data == qb::RLE_FLAG) {
					stream.skip(sizeof(data));
					uint32_t count;
					wrap(stream.readUInt32(count))
				}
				color::RGBA color(0);
				wrapBool(readColor(state, stream, color))
				if (color.a == 0) {
					continue;
				}
				const color::RGBA flattened = flattenRGB(color.r, color.g, color.b);
				colors.put(flattened, true);
			}
			++z;
		}
	}
	return true;
}

size_t QBFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							 const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return 0;
	}

	State state;
	wrap(stream->readUInt32(state._version))
	uint32_t colorFormat;
	wrap(stream->readUInt32(colorFormat))
	state._colorFormat = (ColorFormat)colorFormat;
	uint32_t zAxisOrientation;
	wrap(stream->readUInt32(zAxisOrientation))
	state._zAxisOrientation = (ZAxisOrientation)zAxisOrientation;
	uint32_t compressed;
	wrap(stream->readUInt32(compressed))
	state._compressed = (Compression)compressed;
	uint32_t visibilityMaskEncoded;
	wrap(stream->readUInt32(visibilityMaskEncoded))
	state._visibilityMaskEncoded = (VisibilityMask)visibilityMaskEncoded;

	uint32_t numMatrices;
	wrap(stream->readUInt32(numMatrices))
	if (numMatrices > 16384) {
		Log::error("Max allowed matrices exceeded: %u", numMatrices);
		return 0;
	}
	palette::RGBABuffer colors;
	colors.reserve(numMatrices * 256);
	for (uint32_t i = 0; i < numMatrices; i++) {
		Log::debug("Loading matrix colors: %u", i);
		if (!readPalette(state, *stream, colors)) {
			Log::error("Failed to load the matrix colors %u", i);
			break;
		}
	}
	return createPalette(colors, palette);
}

bool QBFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
							  scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	State state;
	wrap(stream->readUInt32(state._version))
	uint32_t colorFormat;
	wrap(stream->readUInt32(colorFormat))
	state._colorFormat = (ColorFormat)colorFormat;
	uint32_t zAxisOrientation;
	wrap(stream->readUInt32(zAxisOrientation))
	state._zAxisOrientation = (ZAxisOrientation)zAxisOrientation;
	uint32_t compressed;
	wrap(stream->readUInt32(compressed))
	state._compressed = (Compression)compressed;
	uint32_t visibilityMaskEncoded;
	wrap(stream->readUInt32(visibilityMaskEncoded))
	state._visibilityMaskEncoded = (VisibilityMask)visibilityMaskEncoded;

	uint32_t numMatrices;
	wrap(stream->readUInt32(numMatrices))
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
	palette::PaletteLookup palLookup(palette);
	for (uint32_t i = 0; i < numMatrices; i++) {
		Log::debug("Loading matrix: %u", i);
		if (!readMatrix(state, *stream, sceneGraph, palLookup)) {
			Log::error("Failed to load the matrix %u", i);
			break;
		}
	}
	return true;
}

} // namespace voxelformat

#undef wrap
#undef wrapBool
#undef wrapSave
