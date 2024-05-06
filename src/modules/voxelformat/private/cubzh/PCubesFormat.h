/**
 * @file
 */

#pragma once

#include "CubzhFormat.h"

namespace voxelformat {

/**
 * @ingroup Formats
 */
class PCubesFormat : public CubzhFormat {
protected:
	bool loadPCubes(const core::String &filename, const Header &header, io::SeekableReadStream &stream,
					scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) const;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;

public:
	bool singleVolume() const override {
		return true;
	}
};

} // namespace voxelformat
