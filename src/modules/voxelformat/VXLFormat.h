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
		void fromMat4(const glm::mat4 &in);
		glm::mat4 toMat4() const;
		Type matrix;
	};
private:
	static constexpr int NumNormalsRA2 = 244;
	static constexpr int NumNormalsTS = 36;
	static constexpr size_t MaxNodes = 512;

	struct VXLNodeHeader {
		char name[16];				/* ASCIIZ string - name of section */
		uint32_t id;				/* Node id */
		uint32_t unknown;			/* Always 1 - maybe which palette should be used? */
		uint32_t unknown2;			/* Always 0 or 2 */
	};

	struct VXLNodeBody {
		uint32_t *spanStart;		/* List of span start addresses or -1 - number of node times */
		uint32_t *spanEnd;			/* List of span end addresses  or -1 - number of node times */
		uint8_t *spanData;			/* Byte data for each span length */
	};

	struct VXLPalette {
		uint8_t startPaletteRemap;	/* Always 0x1f - player colors palette remapping? */
		uint8_t endPaletteRemap;	/* Always 0x10 */
		uint8_t palette[256][3]; 	/* 256 colour palette for the voxel in RGB format */
	};

	struct VXLHeader {
		char filetype[16];			/* ASCIIZ string - "Voxel Animation" */
		uint32_t paletteCount;
		uint32_t nodeCount;			/* Number of nodes */
		uint32_t tailerCount;
		uint32_t bodysize;			/* Total size in bytes of all node bodies */
		VXLPalette palette;
	};

	struct VXLNodeFooter {
		uint32_t spanStartOffset;	/* Offset into body section to span start list */
		uint32_t spanEndOffset;		/* Offset into body section to span end list */
		uint32_t spanDataOffset;	/* Offset into body section to span data */
		float scale;				/* Scaling vector for the image */
		VXLMatrix transform;		/* 4x3 right handed matrix - x, y and z axis point right, up and behind */
		// this is the bouding box of the final rendered model. If the size of the bounding box is the same as the
		// below given size of the volume - the scaling value would be exactly one.
		glm::vec3 mins;
		glm::vec3 maxs;

		uint8_t xsize;				/* Width of the voxel node */
		uint8_t ysize;				/* Breadth of the voxel node - this is our z */
		uint8_t zsize;				/* Height of the voxel node - this is our y */
		uint8_t normalType;			/* 2 (TS) or 4 (RedAlert2) - normal encoding - https://xhp.xwis.net/documents/normals_tables.html */
	};

	struct VXLModel {
		VXLHeader header;
		VXLNodeHeader nodeHeaders[MaxNodes];	/* number of node times */
		VXLNodeBody nodeBodies[MaxNodes];		/* number of node times */
		VXLNodeFooter nodeFooters[MaxNodes];	/* number of node times */

		int findNodeByName(const core::String& name) const;
	};

	struct VXLNodeOffset {
		int64_t start;
		int64_t end;
		int64_t data;
	};

	struct HVAHeader {
		// 16 chars
		core::String filename;
		uint32_t numFrames;
		// number of nodes that are animated
		uint32_t numNodes;
		// names of all the nodes (null-terminated and 16 chars max per entry)
		core::String nodeNames[MaxNodes];
		int nodeIds[MaxNodes];
	};

	// transformation matrix for each section
	using HVAFrames = core::DynamicArray<VXLMatrix>;

	// https://ppmforums.com/topic-29369/red-alert-2-file-format-descriptions/
	struct HVAModel {
		HVAHeader header;
		HVAFrames frames[MaxNodes];
	};

	// 802 is the unpadded size of vxl_header
	static constexpr size_t HeaderSize = 802;
	// 28 is the unpadded size of vxl_node_header
	static constexpr size_t NodeHeaderSize = 28;
	// 92 is the unpadded size of vxl_node_tailer
	static constexpr size_t NodeTailerSize = 92;
	static constexpr size_t HeaderBodySizeOffset = 28;
	static constexpr int EmptyColumn = -1;

	// writing
	bool writeNodeBodyEntry(io::SeekableWriteStream& stream, const voxel::RawVolume* volume, uint8_t x, uint8_t y, uint8_t z, uint8_t& skipCount, uint8_t& voxelCount, uint8_t normalType) const;
	bool writeNode(io::SeekableWriteStream& stream, const SceneGraphNode& node, VXLNodeOffset& offsets, uint64_t nodeSectionOffset) const;
	bool writeNodeHeader(io::SeekableWriteStream& stream, const SceneGraphNode& node, uint32_t nodeIdx) const;
	bool writeNodeFooter(io::SeekableWriteStream& stream, const SceneGraphNode& node, const VXLNodeOffset& offsets) const;
	bool writeHeader(io::SeekableWriteStream& stream, uint32_t numNodes, const voxel::Palette &palette);

	// reading
	bool readNodeHeader(io::SeekableReadStream& stream, VXLModel& mdl, uint32_t nodeIdx) const;
	bool readNodeFooter(io::SeekableReadStream& stream, VXLModel& mdl, uint32_t nodeIdx) const;
	bool readNode(io::SeekableReadStream& stream, VXLModel& mdl, uint32_t nodeIdx, SceneGraph& sceneGraph, const voxel::Palette &palette) const;
	bool readNodes(io::SeekableReadStream& stream, VXLModel& mdl, SceneGraph& sceneGraph, const voxel::Palette &palette) const;
	bool readNodeFooters(io::SeekableReadStream& stream, VXLModel& mdl) const;
	bool readNodeHeaders(io::SeekableReadStream& stream, VXLModel& mdl) const;

	bool writeHVAHeader(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph) const;
	bool writeHVAFrames(io::SeekableWriteStream& stream, const SceneGraph& sceneGraph) const;
	bool readHVAHeader(io::SeekableReadStream& stream, HVAHeader& header) const;
	bool readHVAFrames(io::SeekableReadStream& stream, const VXLModel &mdl, HVAModel& file) const;

	/**
	 * @brief Hierarchical Voxel Animation
	 */
	bool loadHVA(const core::String &filename, const VXLModel &mdl, SceneGraph& sceneGraph);
	bool saveHVA(const core::String &filename, const SceneGraph& sceneGraph);

	bool saveVXL(core::DynamicArray<const SceneGraphNode*> &nodes, const core::String &filename, io::SeekableWriteStream& stream);

	bool prepareModel(VXLModel& mdl) const;
	bool readHeader(io::SeekableReadStream& stream, VXLModel& mdl, voxel::Palette &palette);

	bool loadFromFile(const core::String &filename, SceneGraph& sceneGraph, voxel::Palette &palette);

	static glm::mat4 switchYAndZ(const glm::mat4 &in);
protected:
	size_t loadPalette(const core::String &filename, io::SeekableReadStream& stream, voxel::Palette &palette) override;

	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream& stream, SceneGraph &sceneGraph, voxel::Palette &palette) override;

	bool saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream, ThumbnailCreator thumbnailCreator) override;
};

}
