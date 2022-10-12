/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxel {
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
	enum class ColorFormat : uint32_t {
		RGBA = 0,
		BGRA = 1
	};
	enum class ZAxisOrientation : uint32_t {
		LeftHanded = 0,
		RightHanded = 1
	};
	enum class Compression : uint32_t {
		None = 0,
		RLE = 1
	};

	// If set to 0 the A value of RGBA or BGRA is either 0 (invisble voxel) or 255 (visible voxel).
	// If set to 1 the visibility mask of each voxel is encoded into the A value telling your software
	// which sides of the voxel are visible. You can save a lot of render time using this option.
	enum class VisibilityMask : uint32_t {
		AlphaChannelVisibleByValue,
		AlphaChannelVisibleSidesEncoded
	};
	struct State {
		uint32_t _version;
		ColorFormat _colorFormat;
		ZAxisOrientation _zAxisOrientation;
		Compression _compressed;
		VisibilityMask _visibilityMaskEncoded;
	};
	// left shift values for the vis mask for the single faces
	enum class VisMaskSides : uint8_t {
		Invisble,
		Left,
		Right,
		Top,
		Bottom,
		Front,
		Back
	};

	bool readColor(State& state, io::SeekableReadStream& stream, core::RGBA &color);
	voxel::Voxel getVoxel(State& state, io::SeekableReadStream& stream, voxel::PaletteLookup &palLookup);
	bool loadMatrix(State& state, io::SeekableReadStream& stream, SceneGraph& sceneGraph, voxel::PaletteLookup &palLookup);
	bool loadFromStream(io::SeekableReadStream& stream, SceneGraph& sceneGraph);

	bool saveMatrix(io::SeekableWriteStream& stream, const SceneGraphNode& node) const;
	bool loadColors(State& state, io::SeekableReadStream& stream, voxel::Palette &palette);
public:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) override;
	bool loadGroupsRGBA(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, const voxel::Palette &palette) override;
	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
