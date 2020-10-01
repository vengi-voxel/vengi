/**
 * @file
 */

#pragma once

#include "core/collection/Array.h"
#include "core/collection/DynamicArray.h"
#include "voxel/RawVolume.h"
#include "io/File.h"
#include "VoxelVolumes.h"
#include <glm/fwd.hpp>

namespace voxel {

class Mesh;

class VoxFileFormat {
protected:
	core::Array<uint8_t, 256> _palette;
	size_t _paletteSize = 0;

	const glm::vec4& getColor(const Voxel& voxel) const;
	glm::vec4 findClosestMatch(const glm::vec4& color) const;
	uint8_t findClosestIndex(const glm::vec4& color) const;
	/**
	 * @brief Maps a custum palette index to our own 256 color palette by a closest match
	 */
	uint8_t convertPaletteIndex(uint32_t paletteIndex) const;
	RawVolume* merge(const VoxelVolumes& volumes) const;
public:
	virtual ~VoxFileFormat() = default;

	virtual bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) = 0;
	virtual RawVolume* load(const io::FilePtr& file);
	virtual bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) = 0;
	virtual bool save(const RawVolume* volume, const io::FilePtr& file);
};

class MeshExporter : public VoxFileFormat {
protected:
	struct MeshExt {
		MeshExt(voxel::Mesh* mesh, const core::String& name);
		voxel::Mesh* mesh = nullptr;
		core::String name;
	};
	using Meshes = core::DynamicArray<MeshExt>;
	virtual bool saveMeshes(const Meshes& meshes, const io::FilePtr& file, float scale = 1.0f, bool quad = false, bool withColor = true, bool withTexCoords = true) = 0;
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override {
		return false;
	}

	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
