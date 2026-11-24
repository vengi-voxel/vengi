/**
 * @file
 */

#include "XMLUtil.h"

namespace util {

core::String xmlEscape(const char *in) {
	core::String out;
	if (in == nullptr) {
		return out;
	}
	for (const char *p = in; *p != '\0'; ++p) {
		switch (*p) {
		case '&':
			out.append("&amp;");
			break;
		case '<':
			out.append("&lt;");
			break;
		case '>':
			out.append("&gt;");
			break;
		case '"':
			out.append("&quot;");
			break;
		case '\'':
			out.append("&apos;");
			break;
		default:
			out += *p;
			break;
		}
	}
	return out;
}

} // namespace util
