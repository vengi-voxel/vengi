/**
 * @file
 */

#pragma once

#include "core/String.h"

namespace util {

struct Version {
	int majorVersion;
	int minorVersion;

	constexpr Version(int _major, int _minor) : majorVersion(_major), minorVersion(_minor) {
	}

	inline bool operator==(const Version &rhs) const {
		return majorVersion == rhs.majorVersion && minorVersion == rhs.minorVersion;
	}

	inline bool isAtLeast(int major, int minor) {
		return major > majorVersion || (major == majorVersion && minor <= minorVersion);
	}
};

Version parseVersion(const core::String &version);

} // namespace util
