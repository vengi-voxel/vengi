/**
 * @file
 */

#pragma once

#include "core/SharedPtr.h"
#include "core/String.h"
#include <stdint.h>

namespace io {
class WriteStream;
struct FormatDescription;
} // namespace io

namespace core {
class Var;
typedef core::SharedPtr<Var> VarPtr;
} // namespace core

namespace voxelformat {

enum FormatCVarFlag : uint32_t {
	FormatCVarFlag_Load = 1u << 0,
	FormatCVarFlag_Save = 1u << 1,
	FormatCVarFlag_Mesh = 1u << 2,
	FormatCVarFlag_Image = 1u << 3,
	FormatCVarFlag_RGB = 1u << 4,
	FormatCVarFlag_All = 1u << 5,
	FormatCVarFlag_Primary = 1u << 6
};

struct FormatVarMeta {
	static constexpr int MaxFormats = 8;
	static constexpr int MaxValueTitles = 8;

	const char *name;
	uint32_t flags;
	/** Specific formats. Empty if the group flags are enough. */
	const io::FormatDescription *formats[MaxFormats];
	/** Optional int-value labels (index == cvar int value). Null/empty slots are skipped. */
	const char *valueTitles[MaxValueTitles];
};

class FormatConfig {
public:
	static bool init();
	static void writeConfigJson(io::WriteStream &stream, const core::VarPtr &var);

	static const FormatVarMeta *varsMeta();
	static int cvarCount();
	static const FormatVarMeta *findVarMeta(const char *name);
	static inline const FormatVarMeta *findVarMeta(const core::String &name) {
		return findVarMeta(name.c_str());
	}
	/**
	 * @param save @c true for save/export, @c false for load/import
	 * @return @c true if this cvar should be shown for the given format
	 */
	static bool appliesTo(const FormatVarMeta &meta, bool save, const io::FormatDescription &desc);

	/** Mesh save UI: quads are only written for OBJ and PLY. */
	static bool meshSaveSupportsQuads(const io::FormatDescription &desc);
	/** Mesh save UI: STL has no vertex colors or texcoords. */
	static bool meshSaveSupportsColor(const io::FormatDescription &desc);
	static bool meshSaveSupportsTexCoords(const io::FormatDescription &desc);
};

} // namespace voxelformat
