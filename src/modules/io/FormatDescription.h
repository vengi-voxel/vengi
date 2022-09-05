/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Vector.h"
#include <stdint.h>

namespace io {

#define MAX_FORMATDESCRIPTION_EXTENSIONS 8
using FormatDescriptionExtensions = core::Vector<core::String, MAX_FORMATDESCRIPTION_EXTENSIONS>;

struct FormatDescription {
	core::String name;						/**< the name of the format */
	FormatDescriptionExtensions exts;		/**< the file extension - nullptr terminated list - all lower case */
	bool (*isA)(uint32_t magic) = nullptr;	/**< function to check whether a magic byte matches for the format description */
	uint32_t flags = 0u;					/**< flags for user defines properties */

	inline bool valid() const {
		return !name.empty();
	}
	bool operator<(const FormatDescription &rhs) const;
	/**
	 * @brief Return the comma separated wildcard for the extensions of this format description
	 */
	core::String wildCard() const;
	/**
	 * @brief Checks whether any of the format description extensions matches the given one
	 * @note we compare them as lower case extensions - so even if you give an upper case version here,
	 * it might still match
	 */
	bool matchesExtension(const core::String &fileExt) const;
};

static const io::FormatDescription ALL_SUPPORTED {"All supported", {}, nullptr, 0};

/**
 * @param desc a terminated list of @c FormatDescription objects
 * @return a comma separated list of the extension wildcards (e.g. @code *.ext,*.ext2 @endcode)
 */
extern core::String convertToAllFilePattern(const FormatDescription *desc);
/**
 * @return The extension list of the given format description. @code Name (*.ext1,*.ext2) @endcode
 */
extern core::String convertToFilePattern(const FormatDescription &desc);
extern bool isImage(const core::String &file);
extern bool isA(const core::String& file, const FormatDescription *desc);

/**
 * @brief Add additional filter groups like "All Minecraft", "All Qubicle" filters
 */
extern void createGroupPatterns(const FormatDescription *desc, core::DynamicArray<io::FormatDescription> &groups);

namespace format {

const FormatDescription *images();
const FormatDescription *lua();
const FormatDescription* palettes();

} // namespace format

} // namespace io
