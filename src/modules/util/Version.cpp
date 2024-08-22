/**
 * @file
 */

#include "Version.h"

namespace util {

Version parseVersion(const core::String &version) {
	int major = 0;
	int minor = 0;
	int i = 0;
	for (; i < (int)version.size(); i++) {
		if (version[i] == '.') {
			break;
		}
		major = major * 10 + (version[i] - '0');
	}
	i++;
	for (; i < (int)version.size(); i++) {
		minor = minor * 10 + (version[i] - '0');
	}
	return Version(major, minor);
}

} // namespace util
