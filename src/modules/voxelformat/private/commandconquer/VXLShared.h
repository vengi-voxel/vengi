/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/Buffer.h"
#include "math/Mat4x3f.h"
#include "math/Mat4f.h"
#include "math/Vec3f.h"

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
	using Type = math::Mat4x3f;
	VXLMatrix() : matrix(1.0f) {
	}
	void fromVengi(const math::Mat4f &in);
	math::Mat4f toVengi() const;
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
	/**
	 * @brief This is a global scale factor for the whole model, but it's only applied to the translation part of the
	 * HVA animation matrices. The VXL base pose matrix is not affected by this. It's a bit of a misnomer, as it
	 * doesn't scale the geometry itself, but rather the animation translations. The value is typically 1.0/12.0.
	 *
	 * @sa convertHVARead()
	 * @sa https://github.com/vengi-voxel/vengi/issues/537
	 */
	float scale;
	VXLMatrix transform;	  /**< 4x3 right handed matrix - x, y and z axis point right, up and back
							   * @sa @c CoordinateSystem::VXL */
	/**
	 * @brief The mins and maxs values define the bounding box of the voxel data. They are used to calculate the
	 * per-section scale and offset. The voxel data is scaled and translated to fit within this box.
	 *
	 * @sa calcScale()
	 * @sa offset()
	 */
	math::Vec3f mins;
	math::Vec3f maxs;

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
	math::Vec3f calcScale() const;
	/**
	 * @brief Calculates the offset of the voxel data from the origin.
	 * @note y and z are flipped to bring it into vengi space.
	 */
	math::Vec3f offset() const;
	/**
	 * @brief The pivot is the normalized position within the bounding box where the origin (0,0,0) is located.
	 * Formula: pivot = -mins / (maxs - mins)
	 * This gives values typically around 0.5 (centered) but can vary if mins/maxs is adjusted.
	 * @note y and z are flipped to bring it into vengi space.
	 */
	math::Vec3f pivot() const;
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

math::Mat4f convertVXLRead(const VXLMatrix &matrix, const VXLLayerInfo &footer);
VXLMatrix convertVXLWrite(const math::Mat4f &vengiMatrix);
math::Mat4f convertHVARead(const VXLMatrix &matrix, const VXLLayerInfo &footer);
VXLMatrix convertHVAWrite(const math::Mat4f &vengiMatrix);

} // namespace vxl
} // namespace voxelformat
