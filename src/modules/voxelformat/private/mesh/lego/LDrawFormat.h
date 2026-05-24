/**
 * @file
 */

#pragma once

#include "io/CachingArchive.h"
#include "voxelformat/private/mesh/MeshFormat.h"
#include "voxelformat/private/mesh/lego/LegoUtil.h"

namespace voxelformat {

/**
 * @brief LDraw model format (.ldr, .mpd)
 *
 * A .ldr file is a single LDraw model that references parts from the LDraw library.
 * An .mpd file is a multi-part document containing multiple .ldr sections.
 *
 * This format uses the LDraw palette file ldconfig.ldr for color definitions.
 * @see https://www.ldraw.org/article/299
 *
 * Coordinate System
 *
 * LDraw uses LDU (LDraw Units) with a right-handed coordinate system where -Y is "up":
 * - X = right
 * - Y = down (negative Y is visually "up")
 * - Z = towards the viewer
 *
 * Standard dimensions in LDU:
 * - 1 brick width/depth = 20 LDU
 * - 1 brick height = 24 LDU
 * - 1 plate height = 8 LDU
 * - 1 stud diameter = 12 LDU
 *
 * Coordinate Conversion to Vengi
 *
 * Vengi uses a right-handed Y-up system. Converting from LDraw requires negating both Y and Z
 * (equivalent to a 180-degree rotation around X). This preserves handedness and avoids visual
 * mirroring. The conversion is applied as a "sandwich" in resolveSubFile():
 * 1. Negate Y and Z of the vertex (undo the storage convention)
 * 2. Apply the LDraw transform (rotation * pos + translation)
 * 3. Re-negate Y and Z (convert back to vengi space)
 *
 * Triangle winding order is reversed to compensate for the odd reflection (Z negation).
 *
 * File Structure
 *
 * Type-1 lines define sub-file references with a 3x3 rotation matrix and position:
 * @code
 * 1 <color> <x> <y> <z> <a> <b> <c> <d> <e> <f> <g> <h> <i> <file>
 * @endcode
 * Where (x,y,z) is the position and the 9 values form a row-major 3x3 rotation matrix.
 *
 * Type-3 lines are triangles, type-4 lines are quads (both with vertex positions in LDU).
 *
 * @see https://www.ldraw.org/article/218.html
 * @see https://library.ldraw.org/
 *
 * @ingroup Formats
 */
class LDrawFormat : public MeshFormat {
protected:
	bool parseStream(io::SeekableReadStream &stream, Mesh &mesh, legoutil::ColorMap &colors,
					 core::DynamicArray<legoutil::SubFileRef> &subFiles, core::String &name,
					 core::String &author) const;

	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;

	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &meshes,
					const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords) override {
		return false;
	}

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"LDraw Model", "text/plain", {"ldr", "mpd"}, {}, VOX_FORMAT_FLAG_MESH};
		return f;
	}
};

} // namespace voxelformat
