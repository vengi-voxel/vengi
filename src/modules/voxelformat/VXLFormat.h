/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/DynamicArray.h"
#include <glm/matrix.hpp>

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
public:
	// vxl stores row major matrices of 3 rows with 4 columns in each row
	// but we are using column major matrices
	struct VXLMatrix {
		using Type = glm::mat4x3;
		VXLMatrix() : matrix(1.0f) {
		}
		void fromVengi(const glm::mat4 &in);
		glm::mat4 toVengi() const;
		Type matrix;
	};

private:
	static constexpr int NumNormalsRA2 = 244;
	static constexpr int NumNormalsTS = 36;
	static constexpr size_t MaxLayers = 512;

	glm::ivec3 maxSize() const override {
		return glm::ivec3(256);
	}

	struct VXLLayerHeader {
		char name[16];		/**< ASCIIZ string - name of section */
		uint32_t infoIndex; /**< Node id */
		uint32_t unknown;	/**< Always 1 - maybe which palette should be used? */
		uint32_t unknown2;	/**< Always 0 or 2 */
	};

	struct VXLLayerBody {
		uint32_t *spanStart; /**< List of span start addresses or -1 - number of node times */
		uint32_t *spanEnd;	 /**< List of span end addresses  or -1 - number of node times */
		uint8_t *spanData;	 /**< Byte data for each span length */
	};

	struct VXLPalette {
		uint8_t startPaletteRemap; /**< Always 0x10 - player colors palette remapping? */
		uint8_t endPaletteRemap;   /**< Always 0x1f */
		uint8_t palette[256][3];   /**< 256 colour palette for the voxel in RGB format */
	};

	struct VXLHeader {
		char filetype[16]; /**< ASCIIZ string - "Voxel Animation" */
		uint32_t paletteCount;
		uint32_t layerCount; /**< Number of nodes */
		uint32_t layerInfoCount;
		uint32_t dataSize; /**< Total size in bytes of all node bodies */
		VXLPalette palette;
	};

	struct VXLLayerInfo {
		uint32_t spanStartOffset; /**< Offset into body section to span start list */
		uint32_t spanEndOffset;	  /**< Offset into body section to span end list */
		uint32_t spanDataOffset;  /**< Offset into body section to span data */
		float scale;			  /**< Scaling vector for the image */
		VXLMatrix transform;	  /**< 4x3 right handed matrix - x, y and z axis point right, up and back
								   * @sa @c CoordinateSystem::XRightYBackZUp */
		// this is the bounding box of the final rendered model. If the size of the bounding box is the same as the
		// below given size of the volume - the scaling value would be exactly one.
		glm::vec3 mins;
		glm::vec3 maxs;

		uint8_t xsize;		/**< Width of the voxel node */
		uint8_t ysize;		/**< Breadth of the voxel node - this is our z */
		uint8_t zsize;		/**< Height of the voxel node - this is our y */
		uint8_t normalType; /**< 2 (TS) or 4 (RedAlert2) - normal encoding -
							   https://xhp.xwis.net/documents/normals_tables.html */
	};

	struct VXLModel {
		VXLHeader header;
		VXLLayerHeader layerHeaders[MaxLayers]; /**< number of node times */
		VXLLayerBody layerBodies[MaxLayers];	/**< number of node times */
		VXLLayerInfo layerInfos[MaxLayers];		/**< number of node times */

		int findLayerByName(const core::String &name) const;
	};

	struct VXLLayerOffset {
		int64_t start;
		int64_t end;
		int64_t data;
	};

	struct HVAHeader {
		// 16 chars
		core::String filename;
		uint32_t numFrames;
		// number of nodes that are animated
		uint32_t numLayers;
		// names of all the nodes (null-terminated and 16 chars max per entry)
		core::String nodeNames[MaxLayers];
		int layerIds[MaxLayers];
	};

	// transformation matrix for each section
	using HVAFrames = core::DynamicArray<VXLMatrix>;

	// https://ppmforums.com/topic-29369/red-alert-2-file-format-descriptions/
	struct HVAModel {
		HVAHeader header;
		HVAFrames frames[MaxLayers];
	};

	// 802 is the unpadded size of VXLHeader
	static constexpr size_t HeaderSize = 802;
	// 28 is the unpadded size of VXLLayerHeader
	static constexpr size_t LayerHeaderSize = 28;
	// 92 is the unpadded size of VXLLayerInfo
	static constexpr size_t LayerInfoSize = 92;
	static constexpr size_t HeaderBodySizeOffset = 28;
	static constexpr int EmptyColumn = -1;

	// writing
	bool writeLayerBodyEntry(io::SeekableWriteStream &stream, const voxel::RawVolume *volume, uint8_t x, uint8_t y,
							 uint8_t z, uint8_t skipCount, uint8_t voxelCount, uint8_t normalType) const;
	bool writeLayer(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
					const scenegraph::SceneGraphNode &node, VXLLayerOffset &offsets, uint64_t nodeSectionOffset) const;
	bool writeLayerHeader(io::SeekableWriteStream &stream, const scenegraph::SceneGraphNode &node,
						  uint32_t nodeIdx) const;
	bool writeLayerInfo(io::SeekableWriteStream &stream, const scenegraph::SceneGraphNode &node,
						const VXLLayerOffset &offsets) const;
	bool writeHeader(io::SeekableWriteStream &stream, uint32_t numNodes, const voxel::Palette &palette);

	// reading
	bool readLayerHeader(io::SeekableReadStream &stream, VXLModel &mdl, uint32_t nodeIdx) const;
	bool readLayerInfo(io::SeekableReadStream &stream, VXLModel &mdl, uint32_t nodeIdx) const;
	bool readLayer(io::SeekableReadStream &stream, VXLModel &mdl, uint32_t nodeIdx, scenegraph::SceneGraph &sceneGraph,
				   const voxel::Palette &palette) const;
	bool readLayers(io::SeekableReadStream &stream, VXLModel &mdl, scenegraph::SceneGraph &sceneGraph,
					const voxel::Palette &palette) const;
	bool readLayerInfos(io::SeekableReadStream &stream, VXLModel &mdl) const;
	bool readLayerHeaders(io::SeekableReadStream &stream, VXLModel &mdl) const;

	bool writeHVAHeader(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) const;
	bool writeHVAFrames(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph) const;
	bool readHVAHeader(io::SeekableReadStream &stream, HVAHeader &header) const;
	bool readHVAFrames(io::SeekableReadStream &stream, const VXLModel &mdl, HVAModel &file) const;

	/**
	 * @brief Hierarchical Voxel Animation
	 */
	bool loadHVA(const core::String &filename, const VXLModel &mdl, scenegraph::SceneGraph &sceneGraph);
	bool saveHVA(const core::String &filename, const scenegraph::SceneGraph &sceneGraph);

	bool saveVXL(const scenegraph::SceneGraph &sceneGraph,
				 core::DynamicArray<const scenegraph::SceneGraphNode *> &nodes, const core::String &filename,
				 io::SeekableWriteStream &stream);

	bool prepareModel(VXLModel &mdl) const;
	bool readHeader(io::SeekableReadStream &stream, VXLModel &mdl, voxel::Palette &palette);

	bool loadFromFile(const core::String &filename, scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
					  const LoadContext &ctx);

	static void convertRead(glm::mat4 &vengiMatrix, const VXLLayerInfo &footer, bool hva);
	static void convertWrite(VXLMatrix &vxlMatrix, const glm::mat4 &vengiMatrix, const glm::vec3 &localTranslate,
							 bool hva);

protected:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
					   const LoadContext &ctx) override;

	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;

	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override;
};

} // namespace voxelformat
