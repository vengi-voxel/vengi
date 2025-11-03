/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include "core/collection/Vector.h"
#include "io/Stream.h"
#include <string.h>
#include <stdint.h>

namespace io {

struct Magic {
	Magic() {
		data.u32 = 0;
		len = 0;
	}
	Magic(uint32_t u) {
		data.u32 = u;
		len = 4;
	}
	Magic(const char *s) {
		const int l = (int)strlen(s);
		data.u8[0] = l > 0 ? s[0] : 0;
		data.u8[1] = l > 1 ? s[1] : 0;
		data.u8[2] = l > 2 ? s[2] : 0;
		data.u8[3] = l > 3 ? s[3] : 0;
		len = l > 4 ? 4 : l;
	}
	Magic(int a, int b, int c, int d) {
		data.u8[0] = a;
		data.u8[1] = b;
		data.u8[2] = c;
		data.u8[3] = d;
		len = 4;
	}
	union data {
		uint32_t u32;
		uint8_t u8[4];
	} data;
	int len;

	int size() const {
		return len;
	}
};

#define MAX_FORMATDESCRIPTION_EXTENSIONS 8
using FormatDescriptionExtensions = core::Vector<core::String, MAX_FORMATDESCRIPTION_EXTENSIONS>;
#define MAX_FORMATDESCRIPTION_MAGICS 16
using FormatDescriptionMagics = core::Vector<Magic, MAX_FORMATDESCRIPTION_MAGICS>;

#define FORMAT_FLAG_ALL (1 << 0)
#define FORMAT_FLAG_GROUP (1 << 1)
// we also have saving implemented
#define FORMAT_FLAG_SAVE (1 << 2)
#define FORMAT_FLAG_NO_LOAD (1 << 3)

// the format has a built-in renderer shot of the creating software
#define VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED (1 << 8)
// the format has a limited amount of colors or an embedded palette
#define VOX_FORMAT_FLAG_PALETTE_EMBEDDED (1 << 9)
// the format is a mesh format and no direct voxel format
#define VOX_FORMAT_FLAG_MESH (1 << 10)
// the format has support for animation and it is implemented
#define VOX_FORMAT_FLAG_ANIMATION (1 << 11)
// the format is a RGB(A) format and no palette based format - this is used
// for formats where we create palettes from the RGB values.
#define VOX_FORMAT_FLAG_RGB (1 << 12)

struct FormatDescription {
	core::String name;				  /**< the name of the format */
	FormatDescriptionExtensions exts; /**< the file extension - all lower case */
	FormatDescriptionMagics magics;	  /**< magic bytes for the format description */
	uint32_t flags = 0u;			  /**< flags for user defined properties */

	// There are pseudo formats (like 'All supported') that are not valid format descriptions in the sense that they are
	// not standing for a real format. These pseudo formats are used to group other formats together and usually don't
	// have extensions.
	inline bool valid() const {
		return !name.empty() && !exts.empty();
	}
	bool operator<(const FormatDescription &rhs) const;
	bool operator>(const FormatDescription &rhs) const;

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

inline const io::FormatDescription &ALL_SUPPORTED() {
	static const io::FormatDescription v{"All supported", {}, {}, FORMAT_FLAG_ALL};
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
bool isA(const core::String &file, const FormatDescription *desc);
bool isA(const core::String &file, const FormatDescription &desc);
bool isA(const io::FormatDescription &desc, uint32_t magic);
bool isA(const io::FormatDescription &check, const io::FormatDescription &desc, const core::String &ext, uint32_t magic);
uint32_t loadMagic(io::SeekableReadStream &stream);
const io::FormatDescription *getDescription(const core::String &filename, uint32_t magic, const io::FormatDescription *descriptions);
const io::FormatDescription *getDescription(const io::FileDescription &fileDesc, uint32_t magic, const io::FormatDescription *descriptions);

/**
 * @brief Add additional filter groups like "All Minecraft", "All Qubicle" filters
 */
void createGroupPatterns(const FormatDescription *desc, core::DynamicArray<io::FormatDescription> &groups);

namespace format {

const FormatDescription *images();
const FormatDescription *fonts();
const FormatDescription *lua();

FormatDescription png();

void writeJson(io::WriteStream &stream, const io::FormatDescription *desc, const core::StringMap<uint32_t> &flags);
void printJson(const FormatDescription *desc, const core::StringMap<uint32_t> &flags = {});

} // namespace format

} // namespace io
