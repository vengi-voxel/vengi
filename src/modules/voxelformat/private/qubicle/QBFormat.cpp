/**
 * @file
 */

#include "QBFormat.h"
#include "color/Color.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "io/Stream.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/MaterialColor.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "voxel/RawVolume.h"
#include "voxelformat/Format.h"
#include "voxelutil/VolumeVisitor.h"
#include "math/Mat4f.h"
#include "math/Vec4f.h"
#include "math/Vec3u.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Conversions.h"

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
	const math::Vec3i _maxs;

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
			if (!voxel::isAir(voxel.getMaterial())) {
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
	const math::Vec3i &size = region.getDimensionsInVoxels();
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
	// QB stores the grid origin, whereas vengi translation locates the pivot.
	const math::Mat4f worldMatrix = sceneGraph.worldMatrix(node, keyFrameIdx);
	const math::Vec3i offset = math::round(math::Vec3f(worldMatrix * math::Vec4f(region.getLowerCorner(), 1.0f)));
	if (leftHanded) {
		wrapSave(stream.writeInt32(offset.x));
		wrapSave(stream.writeInt32(offset.y));
		wrapSave(stream.writeInt32(offset.z));
	} else {
		wrapSave(stream.writeInt32(offset.z));
		wrapSave(stream.writeInt32(offset.y));
		wrapSave(stream.writeInt32(offset.x));
	}
	MatrixWriter writer(stream, sceneGraph, node, leftHanded, rleCompressed);
	auto func = [&writer](int x, int y, int z, const voxel::Voxel &voxel) { writer.addVoxel(x, y, z, voxel); };
	if (leftHanded) {
		voxelutil::visitVolume<voxelutil::VisitorOrder::ZYX>(*sceneGraph.resolveVolume(node), func, voxelutil::VisitAll());
	} else {
		voxelutil::visitVolume<voxelutil::VisitorOrder::XYZ>(*sceneGraph.resolveVolume(node), func, voxelutil::VisitAll());
	}
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
	const bool leftHanded = core::getVar(cfg::VoxformatQBSaveLeftHanded)->boolVal();
	const ZAxisOrientation orientation = leftHanded ? ZAxisOrientation::LeftHanded : ZAxisOrientation::RightHanded;
	const bool rleCompressed = core::getVar(cfg::VoxformatQBSaveCompressed)->boolVal();
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

bool QBFormat::readHeader(io::SeekableReadStream &stream, State &state, uint32_t &numMatrices) {
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
	wrap(stream.readUInt32(numMatrices))
	if (numMatrices > 16384) {
		Log::error("Max allowed matrices exceeded: %u", numMatrices);
		return false;
	}
	return true;
}

bool QBFormat::readMatrixHeader(io::SeekableReadStream &stream, core::String &name, math::Vec3u &size,
								math::Vec3i &offset) {
	wrapBool(stream.readPascalStringUInt8(name))
	wrap(stream.readUInt32(size.x))
	wrap(stream.readUInt32(size.y))
	wrap(stream.readUInt32(size.z))
	if (math::any(math::lessThan(size, math::Vec3u(1))) || math::any(math::greaterThanEqual(size, math::Vec3u(2048)))) {
		Log::error("Invalid matrix size: %u:%u:%u", size.x, size.y, size.z);
		return false;
	}
	wrap(stream.readInt32(offset.x))
	wrap(stream.readInt32(offset.y))
	wrap(stream.readInt32(offset.z))
	return true;
}

template<class Visitor>
bool QBFormat::readVoxels(State &state, io::SeekableReadStream &stream, const math::Vec3u &size, Visitor &&visit) {
	const uint32_t sliceSize = size.x * size.y;
	for (uint32_t z = 0; z < size.z; ++z) {
		uint32_t index = 0;
		while (state._compressed != Compression::None || index < sliceSize) {
			uint32_t count = 1;
			if (state._compressed != Compression::None) {
				uint32_t data;
				wrap(stream.peekUInt32(data))
				if (data == qb::NEXT_SLICE_FLAG) {
					stream.skip(sizeof(data));
					break;
				}
				if (data == qb::RLE_FLAG) {
					stream.skip(sizeof(data));
					wrap(stream.readUInt32(count))
				}
			}
			if (count == 0 || count > sliceSize - index) {
				Log::error("Invalid QB voxel run of %u voxels at slice index %u", count, index);
				return false;
			}
			color::RGBA color(0);
			wrapBool(readColor(state, stream, color))
			if (color.a != 0) {
				visit(flattenRGB(color.r, color.g, color.b), z, index, count);
			}
			index += count;
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
	uint32_t numMatrices;
	wrapBool(readHeader(*stream, state, numMatrices))
	palette::RGBABuffer colors;
	const auto collect = [&colors](color::RGBA color, uint32_t, uint32_t, uint32_t) { colors.insert(color); };
	for (uint32_t i = 0; i < numMatrices; ++i) {
		ctx.report("matrix", (int)i, (int)numMatrices);
		core::String name;
		math::Vec3u size;
		math::Vec3i offset;
		wrapBool(readMatrixHeader(*stream, name, size, offset))
		wrapBool(readVoxels(state, *stream, size, collect))
	}
	ctx.report("matrix", (int)numMatrices, (int)numMatrices);
	return createPalette(colors, palette);
}

bool QBFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
						  scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	State state;
	uint32_t numMatrices;
	wrapBool(readHeader(*stream, state, numMatrices))
	sceneGraph.reserve(numMatrices);
	const bool leftHanded = state._zAxisOrientation == ZAxisOrientation::LeftHanded;
	const bool createVolumePalettes = core::getVar(cfg::VoxelCreatePalette)->boolVal();
	for (uint32_t i = 0; i < numMatrices; ++i) {
		ctx.report("matrix", (int)i, (int)numMatrices);
		core::String name;
		math::Vec3u size;
		math::Vec3i offset;
		wrapBool(readMatrixHeader(*stream, name, size, offset))

		palette::Palette palette;
		if (createVolumePalettes) {
			io::ScopedStreamPos streamPos(*stream);
			palette::RGBABuffer colors;
			const auto collect = [&colors](color::RGBA color, uint32_t, uint32_t, uint32_t) { colors.insert(color); };
			wrapBool(readVoxels(state, *stream, size, collect))
			createPalette(colors, palette);
		}
		if (palette.colorCount() == 0) {
			palette = voxel::getPalette();
		}
		const math::Vec3u dimensions = leftHanded ? size : math::Vec3u(size.z, size.y, size.x);
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.createVolume(voxel::Region::fromSize(math::Vec3i(dimensions)));
		palette::PaletteLookup colors(palette, palette::PaletteLookup::Precision::Exact);
		const auto place = [&](color::RGBA color, uint32_t z, uint32_t index, uint32_t count) {
			const voxel::Voxel voxel = voxel::createVoxel(palette, colors.findClosestIndex(color));
			for (uint32_t n = index; n < index + count; ++n) {
				const int x = (int)(n % size.x);
				const int y = (int)(n / size.x);
				if (leftHanded) {
					node.volume()->setVoxel(x, y, (int)z, voxel);
				} else {
					node.volume()->setVoxel((int)z, y, x, voxel);
				}
			}
		};
		wrapBool(readVoxels(state, *stream, size, place))
		node.setName(name);
		node.setPalette(palette);
		node.transform(0).setLocalTranslation(leftHanded ? offset : math::Vec3i(offset.z, offset.y, offset.x));
		if (sceneGraph.emplace(core::move(node)) == InvalidNodeId) {
			return false;
		}
	}
	ctx.report("matrix", (int)numMatrices, (int)numMatrices);
	sceneGraph.updateTransforms();
	return true;
}

} // namespace voxelformat

#undef wrap
#undef wrapBool
#undef wrapSave
