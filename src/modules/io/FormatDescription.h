/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/Vector.h"
#include <stdint.h>

namespace io {

struct FormatDescription {
	core::String name;						/**< the name of the format */
	core::Vector<core::String, 8> exts;		/**< the file extension - nullptr terminated list */
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

/**
 * @param desc a terminated list of @c FormatDescription objects
 * @return a comma separated list of the extension wildcards (e.g. @code *.ext,*.ext2 @endcode)
 */
extern core::String convertToAllFilePattern(const FormatDescription *desc);
/**
 * @return @code Name (*.ext1,*.ext2) @endcode
 */
extern core::String convertToFilePattern(const FormatDescription &desc);
extern bool isImage(const core::String &file);

namespace format {

const FormatDescription *images();
const FormatDescription *lua();

} // namespace format

} // namespace io
