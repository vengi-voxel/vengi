/**
 * @file
 */

#pragma once

#include "voxelformat/private/mesh/MeshFormat.h"

namespace voxelformat {

/**
 * @brief LEGO Digital Designer model format (.lxf, .lxfml)
 *
 * An .lxf file is a zip archive containing IMAGE100.lxfml (XML model) and IMAGE100.png (thumbnail).
 * The LXFML XML describes part placements via designID, materials, and transformation matrices.
 * Part geometry is resolved from the LDraw parts library using the same sub-file resolution as LDrawFormat.
 *
 * Coordinate System (LDD)
 *
 * LDD uses a right-handed Y-up coordinate system with approximate centimetre units:
 * - Brick width (1 stud) = 0.8 LDD units
 * - Brick height = 0.96 LDD units
 * - Plate height = 0.32 LDD units
 * - Part origins are typically at the bottom center of the top stud
 *
 * Conversion to LDraw LDU: multiply positions by 25 (20 LDU per 0.8 LDD stud width).
 *
 * Transformation Attribute
 *
 * The bone/rigid/camera `transformation` attribute contains 12 comma-separated floats forming
 * a row-major 3x4 affine matrix:
 * @code
 * | r00 r01 r02 |   row 0 of 3x3 rotation
 * | r10 r11 r12 |   row 1
 * | r20 r21 r22 |   row 2
 * | tx  ty  tz  |   translation (LDD world space)
 * @endcode
 *
 * Each `<Bone>` under a `<Part>` holds the world-space placement of that part instance.
 * Optional legacy offsets on `<Part>` (tx, ty, tz attributes) are applied as an extra
 * translation before the bone matrix.
 *
 * @see https://wiki.ldraw.org/wiki/LXF
 *
 * @ingroup Formats
 */
class LXFFormat : public MeshFormat {
protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;

	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &meshes,
					const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords) override {
		return false;
	}

public:
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"LEGO Digital Designer",
									   "application/xml",
									   {"lxf", "lxfml"},
									   {},
									   VOX_FORMAT_FLAG_MESH | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
