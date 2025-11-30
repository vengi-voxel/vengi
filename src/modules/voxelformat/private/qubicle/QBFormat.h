/**
 * @file
 */

#pragma once

#include "voxelformat/Format.h"

namespace palette {
class PaletteLookup;
}

namespace voxelformat {

/**
 * @brief Qubicle Binary (qb) format.
 *
 * https://getqubicle.com/qubicle/documentation/docs/file/qb/
 *
 * @see QBTFormat
 * @see QBCLFormat
 * @see QEFFormat
 *
 * @ingroup Formats
 */
class QBFormat : public RGBAFormat {
private:
	enum class ColorFormat : uint32_t { RGBA = 0, BGRA = 1 };
	enum class ZAxisOrientation : uint32_t { LeftHanded = 0, RightHanded = 1 };
	enum class Compression : uint32_t { None = 0, RLE = 1 };

	// If set to 0 the A value of RGBA or BGRA is either 0 (invisible voxel) or 255 (visible voxel).
	// If set to 1 the visibility mask of each voxel is encoded into the A value telling your software
	// which sides of the voxel are visible. You can save a lot of render time using this option.
	enum class VisibilityMask : uint32_t { AlphaChannelVisibleByValue, AlphaChannelVisibleSidesEncoded };
	struct State {
		uint32_t _version;
		ColorFormat _colorFormat;
		ZAxisOrientation _zAxisOrientation;
		Compression _compressed;
		VisibilityMask _visibilityMaskEncoded;
	};
	// left shift values for the vis mask for the single faces
	enum class VisMaskSides : uint8_t { Invisble, Left, Right, Top, Bottom, Front, Back };

	bool readColor(State &state, io::SeekableReadStream &stream, color::RGBA &color);
	voxel::Voxel getVoxel(State &state, io::SeekableReadStream &stream, palette::PaletteLookup &palLookup);
	bool readMatrix(State &state, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
					palette::PaletteLookup &palLookup);
	bool readPalette(State &state, io::SeekableReadStream &stream, palette::RGBABuffer &colors);
	bool loadGroupsRGBA(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette,
						const LoadContext &ctx) override;
	bool saveMatrix(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
					const scenegraph::SceneGraphNode &node, bool leftHanded, bool rleCompressed) const;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Qubicle Binary", {"qb"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
