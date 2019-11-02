/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/FileStream.h"

namespace voxel {

/**
 * @brief Qubicle Binary (qb) format.
 *
 * http://minddesk.com/learn/article.php?id=22
 */
class QBFormat : public VoxFileFormat {
private:
	uint32_t _version;
	enum class ColorFormat : uint32_t {
		RGBA = 0,
		BGRA = 1
	};
	ColorFormat _colorFormat;
	enum class ZAxisOrientation : uint32_t {
		Left = 0,
		Right = 1
	};
	ZAxisOrientation _zAxisOrientation;
	enum class Compression : uint32_t {
		None = 0,
		RLE = 1
	};
	Compression _compressed;

	// If set to 0 the A value of RGBA or BGRA is either 0 (invisble voxel) or 255 (visible voxel).
	// If set to 1 the visibility mask of each voxel is encoded into the A value telling your software
	// which sides of the voxel are visible. You can save a lot of render time using this option.
	enum class VisibilityMask : uint32_t {
		AlphaChannelVisibleByValue,
		AlphaChannelVisibleSidesEncoded
	};
	VisibilityMask _visibilityMaskEncoded;
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

	void setVoxel(voxel::RawVolume* volume, uint32_t x, uint32_t y, uint32_t z, const glm::ivec3& offset, const voxel::Voxel& voxel);
	voxel::Voxel getVoxel(io::FileStream& stream);
	bool loadMatrix(io::FileStream& stream, VoxelVolumes& volumes);
	bool loadFromStream(io::FileStream& stream, VoxelVolumes& volumes);

	bool saveMatrix(io::FileStream& stream, const VoxelVolume& volume) const;
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
