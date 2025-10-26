/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/Buffer.h"
#include <glm/matrix.hpp>

namespace voxelformat {
namespace vxl {

static constexpr int NumNormalsRA2 = 244;
static constexpr int NumNormalsTS = 36;
static constexpr size_t MaxLayers = 512;
// 802 is the unpadded size of VXLHeader
static constexpr size_t HeaderSize = 802;
// 28 is the unpadded size of VXLLayerHeader
static constexpr size_t LayerHeaderSize = 28;
// 92 is the unpadded size of VXLLayerInfo
static constexpr size_t LayerInfoSize = 92;
static constexpr size_t HeaderBodySizeOffset = 28;
static constexpr int EmptyColumn = -1;
static constexpr float Scale = 1.0f / 12.0f;

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
							   * @sa @c CoordinateSystem::VXL */
	// this is the bounding box of the final rendered model. If the size of the bounding box is the same as the
	// below given size of the volume - the scaling value would be exactly one.
	// The mins and maxs values define the bounding box of the voxel data. They are used to calculate the per-section
	// scale and offset. The voxel data is scaled and translated to fit within this box.
	glm::vec3 mins;
	glm::vec3 maxs;

	uint8_t xsize;		/**< Width of the voxel section */
	uint8_t ysize;		/**< Breadth of the voxel section (in-engine Z) */
	uint8_t zsize;		/**< Height of the voxel section (in-engine Y) */
	uint8_t normalType; /**< 2 (TS) or 4 (RedAlert2) - normal encoding -
						   https://xhp.xwis.net/documents/normals_tables.html */

	/**
	 * @brief Calculates the per-section scale factor from the bounding box.
	 * This scale represents how much the voxel canvas was "shrunk" or "scaled" when saved.
	 * Formula: scale = (maxs - mins) / voxel_canvas_size
	 * Each section can have a different scale!
	 * @note y and z are flipped to bring it into vengi space.
	 */
	glm::vec3 calcScale() const;
	/**
	 * @brief Calculates the offset of the voxel data from the origin.
	 * @note y and z are flipped to bring it into vengi space.
	 */
	glm::vec3 offset() const;
	/**
	 * @brief The pivot is the normalized position within the bounding box where the origin (0,0,0) is located.
	 * Formula: pivot = -mins / (maxs - mins)
	 * This gives values typically around 0.5 (centered) but can vary if mins/maxs is adjusted.
	 * @note y and z are flipped to bring it into vengi space.
	 */
	glm::vec3 pivot() const;
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
using HVAFrames = core::Buffer<VXLMatrix>;

// https://ppmforums.com/topic-29369/red-alert-2-file-format-descriptions/
struct HVAModel {
	HVAHeader header;
	HVAFrames frames[MaxLayers];
};

glm::mat4 convertVXLRead(const VXLMatrix &matrix, const VXLLayerInfo &footer);
VXLMatrix convertVXLWrite(const glm::mat4 &vengiMatrix);
glm::mat4 convertHVARead(const VXLMatrix &matrix, const VXLLayerInfo &footer);
VXLMatrix convertHVAWrite(const glm::mat4 &vengiMatrix);

} // namespace vxl
} // namespace voxelformat
