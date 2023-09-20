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

#define FORMAT_FLAG_ALL (1 << 0)
#define FORMAT_FLAG_GROUP (1 << 1)

#define VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED (1 << 8)
#define VOX_FORMAT_FLAG_PALETTE_EMBEDDED (1 << 9)
#define VOX_FORMAT_FLAG_MESH (1 << 10)

struct FormatDescription {
	core::String name;						/**< the name of the format */
	FormatDescriptionExtensions exts;		/**< the file extension - nullptr terminated list - all lower case */
	bool (*isA)(uint32_t magic) = nullptr;	/**< function to check whether a magic byte matches for the format description */
	uint32_t flags = 0u;					/**< flags for user defined properties */

	// There are pseudo formats (like 'All supported') that are not valid format descriptions in the sense that they are not standing for a real format.
	// These pseudo formats are used to group other formats together and usually don't have extensions.
	inline bool valid() const {
		return !name.empty() && !exts.empty();
	}
	bool operator<(const FormatDescription &rhs) const;

	bool operator==(const FormatDescription &rhs) const {
		if (name.empty() || rhs.name.empty()) {
			if (rhs.exts.empty()) {
				return false;
			}
			return matchesExtension(rhs.exts[0]);
		}
		return name == rhs.name;
	}

	core::String mainExtension(bool includeDot = false) const;

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

struct FileDescription {
	core::String name;
	io::FormatDescription desc;

	void set(const core::String &s, const io::FormatDescription *f = nullptr);

	void clear();

	inline bool empty() const {
		return name.empty();
	}

	inline const char *c_str() const {
		return name.c_str();
	}
};

inline const io::FormatDescription& ALL_SUPPORTED() {
	static const io::FormatDescription v{"All supported", {}, nullptr, FORMAT_FLAG_ALL};
	return v;
}

/**
 * @param desc a terminated list of @c FormatDescription objects
 * @return a comma separated list of the extension wildcards (e.g. @code *.ext,*.ext2 @endcode)
 */
core::String convertToAllFilePattern(const FormatDescription *desc);
/**
 * @return The extension list of the given format description. @code Name (*.ext1,*.ext2) @endcode
 */
core::String convertToFilePattern(const FormatDescription &desc);
bool isImage(const core::String &file);
bool isA(const core::String& file, const FormatDescription *desc);

/**
 * @brief Add additional filter groups like "All Minecraft", "All Qubicle" filters
 */
void createGroupPatterns(const FormatDescription *desc, core::DynamicArray<io::FormatDescription> &groups);

namespace format {

const FormatDescription *images();
const FormatDescription* fonts();
const FormatDescription *lua();
const FormatDescription* palettes();

} // namespace format

} // namespace io
