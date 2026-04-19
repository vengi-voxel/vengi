/**
 * @file
 */

#pragma once

#include "core/collection/Map.h"
#include "io/CachingArchive.h"
#include "voxelformat/private/mesh/MeshFormat.h"

namespace voxelformat {

/**
 * @brief LDraw model format (.ldr)
 *
 * A .ldr file is a single LDraw model that references parts from the LDraw library
 *
 * This format is also using a lego palette file which is called ldconfig.ldr - see
 * https://www.ldraw.org/article/299
 *
 * LDraw uses LDU (LDraw Units) where:
 * - 1 brick width/depth = 20 LDU
 * - 1 brick height = 24 LDU
 * - 1 plate height = 8 LDU
 * - 1 stud diameter = 12 LDU
 * - The coordinate system is right-handed with -Y being "up"
 *
 * @see https://www.ldraw.org/article/218.html
 *
 * Find the tiles library at https://library.ldraw.org/
 *
 * @ingroup Formats
 */
class LDrawFormat : public MeshFormat {
protected:
	using ColorMap = core::Map<int, color::RGBA, 64>;

	static void initColors(ColorMap &colors);
	static void parseColorMeta(const char *line, ColorMap &colors);
	static void parseLDConfig(io::CachingArchive &archive, ColorMap &colors);
	static color::RGBA lookupColor(const ColorMap &colors, int colorCode);

	struct SubFileRef {
		int colorCode;
		glm::vec3 pos;
		glm::mat3 transform;
		core::String filename;
	};

	static void parseLine(const char *line, Mesh &mesh, ColorMap &colors, core::DynamicArray<SubFileRef> &subFiles,
						  core::String &name, core::String &author);

	bool resolveSubFile(io::CachingArchive &archive, const SubFileRef &ref,
						const ColorMap &colors, Mesh &outMesh, int depth) const;

	bool parseStream(io::SeekableReadStream &stream, Mesh &mesh, ColorMap &colors,
					 core::DynamicArray<SubFileRef> &subFiles, core::String &name, core::String &author) const;

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
