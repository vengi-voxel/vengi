/**
 * @file
 */

#pragma once

#include "VXLShared.h"
#include "palette/NormalPalette.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Tiberian Sun Voxel Animation Format
 *
 * @li http://xhp.xwis.net/documents/VXL_Format.txt
 *
 * The format uses z-up as coordinate system
 * @ingroup Formats
 */
class VXLFormat : public PaletteFormat {
private:
	glm::ivec3 maxSize() const override {
		return glm::ivec3(255);
	}

	// writing
	bool writeLayerBodyEntry(io::SeekableWriteStream &stream, const voxel::RawVolume *volume, uint8_t x, uint8_t y,
							 uint8_t z, uint8_t skipCount, uint8_t voxelCount, const palette::NormalPalette &normalPalette) const;
	bool writeLayer(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
					const scenegraph::SceneGraphNode &node, vxl::VXLLayerOffset &offsets,
					uint64_t nodeSectionOffset) const;
	bool writeLayerHeader(io::SeekableWriteStream &stream, const scenegraph::SceneGraphNode &node,
						  uint32_t nodeIdx) const;
	bool writeLayerInfo(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
						const scenegraph::SceneGraphNode &node, const vxl::VXLLayerOffset &offsets) const;
	bool writeHeader(io::SeekableWriteStream &stream, uint32_t numNodes, const palette::Palette &palette);

	// reading
	bool readLayerHeader(io::SeekableReadStream &stream, vxl::VXLModel &mdl, uint32_t nodeIdx) const;
	bool readLayerInfo(io::SeekableReadStream &stream, vxl::VXLModel &mdl, uint32_t nodeIdx) const;
	bool readLayer(io::SeekableReadStream &stream, vxl::VXLModel &mdl, uint32_t nodeIdx,
				   scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette) const;
	bool readLayers(io::SeekableReadStream &stream, vxl::VXLModel &mdl, scenegraph::SceneGraph &sceneGraph,
					const palette::Palette &palette) const;
	bool readLayerInfos(io::SeekableReadStream &stream, vxl::VXLModel &mdl) const;
	bool readLayerHeaders(io::SeekableReadStream &stream, vxl::VXLModel &mdl) const;

	bool saveVXL(const scenegraph::SceneGraph &sceneGraph,
				 core::DynamicArray<const scenegraph::SceneGraphNode *> &nodes, const core::String &filename,
				 const io::ArchivePtr &archive);

	bool prepareModel(vxl::VXLModel &mdl) const;
	bool readHeader(io::SeekableReadStream &stream, vxl::VXLModel &mdl, palette::Palette &palette);

protected:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;

	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
					public:

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Tiberian Sun",
									{"vxl"},
									{"Voxel Animation"},
									VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_ANIMATION | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
