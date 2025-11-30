/**
 * @file
 */

#include "CSMFormat.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "io/Archive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxel/Voxel.h"
#include <glm/common.hpp>

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load csm file: Not enough data in stream " CORE_STRINGIFY(read));                        \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::debug("Error: " CORE_STRINGIFY(read) " at " CORE_FILE ":%i", CORE_LINE);                                  \
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

bool CSMFormat::loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
							   const LoadContext &ctx) {
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

	palette::PaletteLookup palLookup(palette);
	for (uint16_t i = 0u; (uint16_t)i < matrixCount; ++i) {
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

		uint32_t voxels = sizex * sizey * sizez;
		uint32_t matrixIndex = 0u;

		voxel::RawVolume *volume = new voxel::RawVolume(region);
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		node.setVolume(volume, true);

		while (matrixIndex < voxels) {
			uint8_t count;
			wrap(stream->readUInt8(count))
			uint8_t r;
			wrap(stream->readUInt8(r))
			uint8_t g;
			wrap(stream->readUInt8(g))
			uint8_t b;
			wrap(stream->readUInt8(b))
			uint8_t interactiontype;
			wrap(stream->readUInt8(interactiontype))
			if (interactiontype == 0u) {
				matrixIndex += count;
				continue;
			}
			const color::RGBA color = flattenRGB(r, g, b);
			const int index = palLookup.findClosestIndex(color);
			const voxel::Voxel &voxel = voxel::createVoxel(palette, index);

			// TODO: PERF: use volume sampler
			for (uint32_t v = matrixIndex; v < matrixIndex + count; ++v) {
				const int x = (int)glm::mod((float)glm::floor((float)v / (float)(sizez * sizey)), (float)sizex);
				const int y = (int)glm::mod((float)glm::floor((float)v / (float)(sizez)), (float)sizey);
				const int z = (int)glm::mod((float)v, (float)sizez);
				volume->setVoxel(x, y, z, voxel);
			}

			matrixIndex += count;
		}
		if (version >= 2) {
			name = makeNameUnique(sceneGraph, name);
		}
		node.setName(name);
		scenegraph::SceneGraphTransform transform;
		if (version >= 4) {
			transform.setWorldTranslation(glm::vec3(posx, posy, posz) / 10.0f);
		} else {
			transform.setWorldTranslation(glm::vec3(posx, posy, posz) / 2.0f);
		}
		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		node.setTransform(keyFrameIdx, transform);

		if (!parent.empty()) {
			node.setProperty(scenegraph::PropParent, parent);
		}
		node.setPalette(palLookup.palette());
		sceneGraph.emplace(core::move(node));
	}
	if (version > 1) {
		updateParents(sceneGraph);
	}
	return true;
}

#undef wrap
#undef wrapBool

bool CSMFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	return false;
}

} // namespace voxelformat
