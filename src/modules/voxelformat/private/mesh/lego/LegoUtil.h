/**
 * @file
 */

#pragma once

#include "core/collection/DynamicMap.h"
#include "core/collection/Map.h"
#include "io/Archive.h"
#include "io/CachingArchive.h"
#include "voxelformat/private/mesh/Mesh.h"

namespace voxelformat {

/**
 * @brief Shared helpers for LDraw and LXF LEGO mesh loading
 *
 * ## Coordinate Conversion
 *
 * LDraw uses a right-handed coordinate system with -Y up. Vengi uses right-handed +Y up.
 * The conversion negates both Y and Z (180-degree rotation around X), which preserves
 * handedness and avoids visual mirroring.
 *
 * ## Sub-File Resolution
 *
 * resolveSubFile() recursively resolves LDraw part references. Each sub-file reference
 * contains a position (vec3) and rotation (mat3) in LDraw space. The transform is applied
 * using a "sandwich" pattern:
 * 1. Convert vertex from vengi space to LDraw space (negate Y and Z)
 * 2. Apply the LDraw transform: rotation * vertex + position
 * 3. Convert back to vengi space (negate Y and Z again)
 *
 * Nested sub-file references compose their transforms in LDraw space before the final
 * conversion to vengi space.
 *
 * ## Color Handling
 *
 * Color code 16 means "inherit from parent". When a sub-file reference uses color 16,
 * it inherits the color from its parent reference. The color lookup supports both
 * standard LDraw color codes and direct RGB encoding (codes >= 0x2000000).
 */
namespace legoutil {

using ColorMap = core::Map<int, color::RGBA, 64>;

struct SubFileRef {
	int colorCode;
	glm::vec3 pos;
	glm::mat3 transform;
	core::String filename;
};

void initColors(ColorMap &colors);
void parseColorMeta(const char *line, ColorMap &colors);
void parseLDConfig(io::CachingArchive &archive, ColorMap &colors);
void parseLegoIdMap(io::SeekableReadStream &stream, ColorMap &colors, core::DynamicMap<int, int> &legoIdToLdrawCode);
color::RGBA lookupColor(const ColorMap &colors, int colorCode);
int lookupLdrawColor(const core::DynamicMap<int, int> &legoIdToLdrawCode, int legoMaterialId);

void parseLine(const char *line, Mesh &mesh, ColorMap &colors, core::DynamicArray<SubFileRef> &subFiles,
			   core::String &name, core::String &author);
bool resolveSubFile(io::CachingArchive &archive, const SubFileRef &ref, const ColorMap &colors, Mesh &outMesh,
					int depth = 0);

io::ArchivePtr openLookupArchive(const io::ArchivePtr &archive);
void registerLdrawSearchPaths(io::CachingArchive &cachedArchive);

} // namespace legoutil

} // namespace voxelformat
