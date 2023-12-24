/**
 * @file
 */

#pragma once

#include "io/Stream.h"
#include "voxelformat/Format.h"

namespace voxelformat {

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
