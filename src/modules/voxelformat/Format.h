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
#include "SceneGraph.h"
#include "voxel/Palette.h"
#include <glm/fwd.hpp>

namespace voxel {

// the max amount of voxels - [0-255]
static constexpr int MaxRegionSize = 256;


class Mesh;

/**
 * @brief Base class for all voxel formats.
 */
class Format {
protected:
	/**
	 * @brief This array contains a mapping between the input colors of the particular format with the internal
	 * currently in-use palette indices.
	 * For example. You are loading a format that e.g. contains RGBA colors in a chunk and your particular file
	 * includes 256 colors as RGBA values. We now try to map the format input RGBA colors values to the ones from
	 * our own currently in-use palette. This is done by searching for a color in the current in-use palette that
	 * is very similar to the color from the input file you are loading. The found index in the in-use palette is
	 * then stored in this array for fast lookup when mapping the voxel data from the internal RGBA index of the
	 * input file to the currently in-use matching palette index.
	 *
	 * @sa findClosestIndex()
	 */
	core::Array<uint8_t, 256> _paletteMapping;
	/**
	 * This is the loaded palette from the input file. This is not the currently in-use palette. This might differ
	 * and the colors will get matched to the in-use palette.
	 */
	Palette _palette;

	/**
	 * @brief Find the closed index in the currently in-use palette for the given color
	 * @param color Normalized color value [0.0-1.0]
	 * @sa core::Color::getClosestMatch()
	 */
	uint8_t findClosestIndex(const glm::vec4& color) const;
	uint8_t findClosestIndex(uint32_t rgba) const;

	/**
	 * @brief Maps a custum palette index to our own 256 color palette by a closest match
	 */
	uint8_t convertPaletteIndex(uint32_t paletteIndex) const;
	RawVolume* merge(const SceneGraph& sceneGraph) const;

	/**
	 * @brief Checks whether the given chunk is empty (only contains air).
	 *
	 * @param v The volume
	 * @param maxSize The chunk size
	 * @param x The chunk position
	 * @param y The chunk position
	 * @param z The chunk position
	 */
	bool isEmptyBlock(const voxel::RawVolume *v, const glm::ivec3 &maxSize, int x, int y, int z) const;
	/**
	 * @brief Calculate the boundaries while aligning them to the given @c maxSize. This ensures that the
	 * calculated extends are exactly @c maxSize when iterating over them (and align relative to 0,0,0 and
	 * @c maxSize).
	 *
	 * @param[in] region The region to calculate the aligned mins/maxs for
	 * @param[in] maxSize The size of a single chunk to align with.
	 * @param[out] mins The extends of the aabb aligned with @c maxSize
	 * @param[out] maxs The extends of the aabb aligned with @c maxSize
	 */
	void calcMinsMaxs(const voxel::Region& region, const glm::ivec3 &maxSize, glm::ivec3 &mins, glm::ivec3 &maxs) const;
	/**
	 * @brief Split volumes according to their max size into several smaller volumes
	 * Some formats only support small volumes sizes per object - but multiple objects.
	 */
	void splitVolumes(const SceneGraph& srcSceneGraph, SceneGraph& destSceneGraph, const glm::ivec3 &maxSize);
	voxel::RawVolume* transformVolume(const SceneGraphTransform &t, const voxel::RawVolume *in) const;

public:
	virtual ~Format() = default;

	virtual image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream& stream);

	/**
	 * @brief Only load the palette that is included in the format
	 * @note Not all voxel formats have a palette included
	 *
	 * @return the amount of colors found in the palette
	 */
	virtual size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, Palette &palette);

	/**
	 * @brief If the format supports multiple layers or groups, this method will give them to you as single volumes
	 */
	virtual bool loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) = 0;
	/**
	 * @brief Merge the loaded volumes into one. The returned memory is yours.
	 */
	virtual RawVolume* load(const core::String &filename, io::SeekableReadStream& stream);
	virtual bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) = 0;
	virtual bool save(const RawVolume* volume, const core::String &filename, io::SeekableWriteStream& stream);
};

}
