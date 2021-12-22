/**
 * @file
 */

#pragma once

#include "core/collection/Array.h"
#include "core/collection/DynamicArray.h"
#include "io/Stream.h"
#include "voxel/RawVolume.h"
#include "io/File.h"
#include "image/Image.h"
#include "VoxelVolumes.h"
#include <glm/fwd.hpp>

namespace voxel {

class Mesh;

/**
 * @brief Base class for all voxel formats.
 */
class Format {
protected:
	core::Array<uint8_t, 256> _palette;
	core::Array<uint32_t, 256> _colors;
	size_t _paletteSize = 0;
	size_t _colorsSize = 0;

	const glm::vec4& getColor(const Voxel& voxel) const;
	glm::vec4 findClosestMatch(const glm::vec4& color) const;
	uint8_t findClosestIndex(const glm::vec4& color) const;
	/**
	 * @brief Maps a custum palette index to our own 256 color palette by a closest match
	 */
	uint8_t convertPaletteIndex(uint32_t paletteIndex) const;
	RawVolume* merge(const VoxelVolumes& volumes) const;
	bool isEmptyBlock(const voxel::RawVolume *v, const glm::ivec3 &maxSize, int x, int y, int z) const;
	void calcMinsMaxs(const voxel::Region& region, const glm::ivec3 &maxSize, glm::ivec3 &mins, glm::ivec3 &maxs) const;
	void split(VoxelVolumes& destVolumes, const VoxelVolume &v, const glm::ivec3& maxSize);
	/**
	 * @brief Split volumes according to their max size into several smaller volumes
	 * Some formats only support small volumes sizes per object - but multiple objects.
	 */
	void splitVolumes(const VoxelVolumes& srcVolumes, VoxelVolumes& destVolumes, const glm::ivec3 &maxSize);

public:
	virtual ~Format() = default;

	virtual image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream);

	/**
	 * @brief Only load the palette that is included in the format
	 * @note Not all voxel formats have a palette included
	 *
	 * @return the amount of colors found in the palette
	 */
	virtual size_t loadPalette(const core::String &filename, io::SeekableReadStream& file, core::Array<uint32_t, 256> &palette);

	/**
	 * @brief If the format supports multiple layers or groups, this method will give them to you as single volumes
	 */
	virtual bool loadGroups(const core::String &filename, io::SeekableReadStream& file, VoxelVolumes& volumes) = 0;
	/**
	 * @brief Merge the loaded volumes into one. The returned memory is yours.
	 */
	virtual RawVolume* load(const core::String &filename, io::SeekableReadStream& file);
	virtual bool saveGroups(const VoxelVolumes& volumes, const core::String &filename, io::SeekableWriteStream& stream) = 0;
	virtual bool save(const RawVolume* volume, const core::String &filename, io::SeekableWriteStream& stream);
};

/**
 * @brief Convert the volume data into a mesh
 */
class MeshExporter : public Format {
protected:
	struct MeshExt {
		MeshExt(voxel::Mesh* mesh, const core::String& name);
		voxel::Mesh* mesh = nullptr;
		core::String name;
	};
	using Meshes = core::DynamicArray<MeshExt>;
	virtual bool saveMeshes(const Meshes& meshes, const core::String &filename, io::SeekableWriteStream& stream, float scale = 1.0f, bool quad = false, bool withColor = true, bool withTexCoords = true) = 0;
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const core::String &filename, io::SeekableWriteStream& stream) override;
};

}
