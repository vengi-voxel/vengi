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
	core::Vector<core::String, 8> exts;					/**< the file extension - nullptr terminated list */
	bool (*isA)(uint32_t magic) = nullptr;	/**< function to check whether a magic byte matches for the format description */
	uint32_t flags = 0u;					/**< flags for user defines properties */

	inline bool valid() const { return !name.empty(); }
	bool operator<(const FormatDescription &rhs) const;
	core::String wildCard() const;
	bool matchesExtension(const core::String &fileExt) const;
};

extern core::String convertToAllFilePattern(const FormatDescription *desc);
extern core::String convertToFilePattern(const FormatDescription &desc);
extern bool isImage(const core::String& file);

namespace format {

const FormatDescription* images();
const FormatDescription* lua();

}

}
