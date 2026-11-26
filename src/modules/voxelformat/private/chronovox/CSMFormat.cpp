/**
 * @file
 */

#include "CSMFormat.h"
#include "core/FourCC.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "io/Archive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "io/StreamUtil.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "math/Vec3f.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load csm file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::debug("Error: %s at %s:%i", CORE_STRINGIFY(read), CORE_FILE, CORE_LINE);                                  \
		return false;                                                                                                  \
	}

static bool readString(io::SeekableReadStream &stream, core::String &str, bool readStringAsInt) {
	if (readStringAsInt) {
		uint32_t length;
		wrap(stream.readUInt32(length))
		if (length >= 4096) {
			// sanity check
			return false;
		}
		char name[4096];
		wrapBool(stream.readString(length, name))
		name[length] = '\0';
		str = name;
	} else {
		uint8_t length;
		wrap(stream.readUInt8(length))
		char name[256];
		wrapBool(stream.readString(length, name))
		name[length] = '\0';
		str = name;
	}
	return true;
}

static void updateParents(scenegraph::SceneGraph &sceneGraph) {
	for (scenegraph::SceneGraph::iterator iter = sceneGraph.beginAll(); iter != sceneGraph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		const core::String &parent = node.property(scenegraph::PropParent);
		if (parent.empty()) {
			Log::debug("no parent for node %s", node.name().c_str());
			continue;
		}
		if (const scenegraph::SceneGraphNode *parentNode = sceneGraph.findNodeByName(parent)) {
			Log::debug("change parent for node %s to %s", node.name().c_str(), parent.c_str());
			sceneGraph.changeParent(node.id(), parentNode->id(), scenegraph::NodeMoveFlag::None);
		} else {
			Log::warn("Failed to find parent node '%s' for node '%s'", parent.c_str(), node.name().c_str());
		}
	}
}

static core::String makeNameUnique(const scenegraph::SceneGraph &sceneGraph, core::String name) {
	while (sceneGraph.findNodeByName(name) != nullptr) {
		name.append("+");
	}
	return name;
}

bool CSMFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}
	uint32_t magic, version, blank, matrixCount;
	wrap(stream->readUInt32(magic))
	const bool isNVM = magic == FourCC('.', 'N', 'V', 'M');
	wrap(stream->readUInt32(version))
	wrap(stream->readUInt32(blank))
	wrap(stream->readUInt32(matrixCount))
	Log::debug("CSM version: %i", version);

	if (isNVM && version > 2) {
		Log::warn("nvm is only supported up to version 2");
	}
	if (!isNVM && version > 4) {
		Log::warn("csm is only supported up to version 4");
	}

	const bool readStringAsInt = isNVM || version >= 4;

	const bool createVolumePalettes = core::getVar(cfg::VoxelCreatePalette)->boolVal();
	for (uint32_t i = 0; i < matrixCount; ++i) {
		ctx.report("matrix", (int)i, (int)matrixCount);
		core::String name;
		core::String parent;
		wrapBool(readString(*stream, name, readStringAsInt))
		if (version > 1) {
			wrapBool(readString(*stream, parent, readStringAsInt))
		}
		int16_t posx, posy, posz;
		wrap(stream->readInt16(posx))
		wrap(stream->readInt16(posy))
		wrap(stream->readInt16(posz))

		uint16_t sizex, sizey, sizez;
		wrap(stream->readUInt16(sizex))
		wrap(stream->readUInt16(sizey))
		wrap(stream->readUInt16(sizez))

		if (sizex > MaxRegionSize || sizey > MaxRegionSize || sizez > MaxRegionSize) {
			Log::error("Volume exceeds the max allowed size: %i:%i:%i", sizex, sizey, sizez);
			return false;
		}

		const voxel::Region region(0, 0, 0, sizex - 1, sizey - 1, sizez - 1);
		if (!region.isValid()) {
			Log::error("Invalid region: %i:%i:%i", sizex, sizey, sizez);
			return false;
		}

		const uint32_t planeSize = sizez * sizey;
		const uint32_t voxelCount = sizex * planeSize;
		const auto readRuns = [&](auto &&visit) {
			for (uint32_t matrixIndex = 0; matrixIndex < voxelCount;) {
				uint8_t count;
				wrap(stream->readUInt8(count))
				color::RGBA rgba;
				wrapBool(io::readColor(*stream, rgba))
				if (count == 0 || count > voxelCount - matrixIndex) {
					Log::error("Invalid Chronovox voxel run");
					return false;
				}
				if (rgba.a != 0) {
					visit(flattenRGB(rgba.r, rgba.g, rgba.b), matrixIndex, count);
				}
				matrixIndex += count;
			}
			return true;
		};

		palette::Palette palette;
		if (createVolumePalettes) {
			io::ScopedStreamPos streamPos(*stream);
			palette::RGBABuffer colors;
			wrapBool(readRuns([&](color::RGBA color, uint32_t, uint8_t) { colors.insert(color); }))
			createPalette(colors, palette);
		}
		if (palette.colorCount() == 0) {
			palette = voxel::getPalette();
		}

		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.createVolume(region);
		palette::PaletteLookup lookup(palette, palette::PaletteLookup::Precision::Exact);
		wrapBool(readRuns([&](color::RGBA color, uint32_t start, uint8_t count) {
			const voxel::Voxel voxel = voxel::createVoxel(palette, lookup.findClosestIndex(color));
			for (uint32_t index = start; index < start + count; ++index) {
				node.volume()->setVoxel((int)(index / planeSize), (int)(index / sizez % sizey), (int)(index % sizez), voxel);
			}
		}))
		if (version >= 2) {
			name = makeNameUnique(sceneGraph, name);
		}
		node.setName(name);
		scenegraph::SceneGraphTransform transform;
		if (version >= 4) {
			transform.setWorldTranslation(math::Vec3f(posx, posy, posz) / 10.0f);
		} else {
			transform.setWorldTranslation(math::Vec3f(posx, posy, posz) / 2.0f);
		}
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		node.setTransform(keyFrameIdx, transform);

		if (!parent.empty()) {
			node.setProperty(scenegraph::PropParent, parent);
		}
		node.setPalette(palette);
		sceneGraph.emplace(core::move(node));
	}
	ctx.report("matrix", (int)matrixCount, (int)matrixCount);
	if (version > 1) {
		updateParents(sceneGraph);
	}
	sceneGraph.updateTransforms();
	return true;
}

#undef wrap
#undef wrapBool

bool CSMFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	return false;
}

} // namespace voxelformat
