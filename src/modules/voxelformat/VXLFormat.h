/**
 * @file
 */

#pragma once

#include "VoxFileFormat.h"
#include "core/io/FileStream.h"

namespace voxel {

/**
 * @brief Tiberian Sun Voxel Animation Format
 *
 * http://xhp.xwis.net/documents/VXL_Format.txt
 */
class VXLFormat : public VoxFileFormat {
private:
	static constexpr size_t MaxLimbs = 512;
	static constexpr size_t MaxPaletteColors = 256;
	struct vxl_limb_header {
		char limb_name[16];			/* ASCIIZ string - name of section */
		uint32_t limb_number;		/* Limb number */
		uint32_t unknown;			/* Always 1 */
		uint32_t unknown2;			/* Always 0 */
	};
	struct vxl_limb_body {
		uint32_t *span_start;		/* List of span start addresses or -1 - number of limb times */
		uint32_t *span_end;			/* List of span end addresses  or -1 - number of limb times */
		uint8_t *span_data;			/* Byte data for each span length */
	};
	struct vxl_header {
		char filetype[16];					  /* ASCIIZ string - "Voxel Animation" */
		uint32_t unknown;					  /* Always 1 - number of animation frames? */
		uint32_t n_limbs;					  /* Number of limb headers/bodies/tailers */
		uint32_t n_limbs2;					  /* Always the same as n_limbs */
		uint32_t bodysize;					  /* Total size in bytes of all limb bodies */
		uint16_t unknown2;					  /* Always 0x1f10 - ID or end of header code? */
		uint8_t palette[MaxPaletteColors][3]; /* 256 colour palette for the voxel in RGB format */
	};
	struct vxl_limb_tailer {
		uint32_t span_start_off;	/* Offset into body section to span start list */
		uint32_t span_end_off;		/* Offset into body section to span end list */
		uint32_t span_data_off;		/* Offset into body section to span data */
		float transform[4][4];		/* Inverse(?) transformation matrix */
		float scale[3];				/* Scaling vector for the image */
		uint8_t xsize;				/* Width of the voxel limb */
		uint8_t ysize;				/* Breadth of the voxel limb */
		uint8_t zsize;				/* Height of the voxel limb */
		uint8_t type;				/* Always 2? type? unknown */
	};
	struct vxl_mdl {
		vxl_header header;
		vxl_limb_header limb_headers[MaxLimbs];	/* number of limb times */
		vxl_limb_body limb_bodies[MaxLimbs];	/* number of limb times */
		vxl_limb_tailer limb_tailers[MaxLimbs];	/* number of limb times */
		int volumeIdx = 0;
	};

	struct LimbOffset {
		int64_t start;
		int64_t end;
		int64_t data;
	};

	// 802 is the unpadded size of vxl_header
	static constexpr size_t HeaderSize = 802;
	// 28 is the unpadded size of vxl_limb_header
	static constexpr size_t LimbHeaderSize = 28;
	// 92 is the unpadded size of vxl_limb_tailer
	static constexpr size_t LimbTailerSize = 92;
	static constexpr size_t HeaderBodySizeOffset = 28;
	static constexpr int EmptyColumn = -1;

	// writing
	bool writeLimbBodyEntry(io::FileStream& stream, voxel::RawVolume* volume, uint8_t x, uint8_t y, uint8_t z, uint32_t& skipCount, uint32_t& voxelCount) const;
	bool writeLimb(io::FileStream& stream, const VoxelVolumes& volumes, uint32_t limbIdx, LimbOffset& offsets, uint64_t limbSectionOffset) const;
	bool writeLimbHeader(io::FileStream& stream, const VoxelVolumes& volumes, uint32_t limbIdx) const;
	bool writeLimbFooter(io::FileStream& stream, const VoxelVolumes& volumes, uint32_t limbIdx, const LimbOffset& offsets) const;
	bool writeHeader(io::FileStream& stream, const VoxelVolumes& volumes);

	// reading
	bool readLimbHeader(io::FileStream& stream, vxl_mdl& mdl, uint32_t limbIdx) const;
	bool readLimbFooter(io::FileStream& stream, vxl_mdl& mdl, uint32_t limbIdx) const;
	bool readLimb(io::FileStream& stream, vxl_mdl& mdl, uint32_t limbIdx, VoxelVolumes& volumes) const;
	bool readLimbs(io::FileStream& stream, vxl_mdl& mdl, VoxelVolumes& volumes) const;
	bool readLimbFooters(io::FileStream& stream, vxl_mdl& mdl) const;
	bool readLimbHeaders(io::FileStream& stream, vxl_mdl& mdl) const;
	bool prepareModel(vxl_mdl& mdl) const;
	bool readHeader(io::FileStream& stream, vxl_mdl& mdl);
public:
	bool loadGroups(const io::FilePtr& file, VoxelVolumes& volumes) override;
	bool saveGroups(const VoxelVolumes& volumes, const io::FilePtr& file) override;
};

}
