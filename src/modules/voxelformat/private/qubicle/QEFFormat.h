/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Qubicle Exchange (QEF)
 *
 * QEF is a rather old ASCII exchange format originally developed for the unreleased Qubicle Plugin for Maya. It is
 * recommended to use the newer and more flexible Qubicle Binary exchange format instead.
 *
 * @see QBTFormat
 * @see QBFormat
 *
 * https://getqubicle.com/qubicle/documentation/docs/file/qef/
 *
 * @ingroup Formats
 */
class QEFFormat : public PaletteFormat {
protected:
	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;

	bool singleVolume() const override {
		return true;
	}
};

} // namespace voxelformat
