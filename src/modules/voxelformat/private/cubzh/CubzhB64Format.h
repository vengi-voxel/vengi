/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @li v0: lua table serialized
 * @li v1: versionId, map chunk that can be read from cpp to load map, then 3 table serialized as base64 chunks
 * @li v2: versionId, map chunk that can be read from cpp to load map, ambience fields, objects, blocks
 *     ambience, objects and blocks might not be serialized if the value is nil or length is 0
 * @li v3: same as v2 but removed itemDetailsCell and Objects chunk length is now uint32 and not uint16
 */
class CubzhB64Format : public RGBAFormat {
protected:
	bool readObjects(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					 const LoadContext &ctx, int version);
	bool readChunkMap(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					  const LoadContext &ctx);
	bool readAmbience(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					  const LoadContext &ctx);
	bool readBlocks(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					const LoadContext &ctx);
	bool loadVersion1(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					  const LoadContext &ctx);
	bool loadVersion2(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					  const LoadContext &ctx);
	bool loadVersion3(io::ReadStream &stream, scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
					  const LoadContext &ctx);
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream &stream,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const LoadContext &ctx);
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) {
		return false;
	}
};

} // namespace voxelformat
