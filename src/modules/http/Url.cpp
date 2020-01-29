/**
 * @file
 */

#include "Url.h"
#include "core/StringUtil.h"
#include "core/Log.h"

namespace http {

void Url::parseSchema(char **strPtr) {
	char* str = *strPtr;
	char* pos = SDL_strchr(str, ':');
	if (pos == nullptr) {
		_valid = false;
		return;
	}
	schema = std::string(str, pos - str);
	++pos;
	for (int i = 0; i < 2; ++i) {
		if (*pos++ != '/') {
			_valid = false;
			break;
		}
	}
	*strPtr = pos;
}

void Url::parseHostPart(char **strPtr) {
	char* str = *strPtr;
	char* endOfHostPort = SDL_strchr(str, '/');
	size_t hostPortLength;
	if (endOfHostPort == nullptr) {
		hostPortLength = (size_t)strlen(str);
	} else {
		hostPortLength = (size_t)(intptr_t)(endOfHostPort - str);
		*strPtr += hostPortLength + 1;
	}

	bool hasUser = false;
	for (size_t i = 0u; i < hostPortLength; ++i) {
		if (str[i] == '@') {
			hasUser = true;
			break;
		}
	}

	char *p = str;
	if (hasUser) {
		while (*p != '\0' && *p != ':' && *p != '@') {
			++p;
		}
		username = std::string(str, p - str);
		if (*p == ':') {
			str = ++p;
			while (*p != '\0' && *p != '@') {
				++p;
			}
			password = std::string(str, p - str);
		}
		if (*p != '\0') {
			++p;
		}
	}

	char *afterUser = p;
	for (;;) {
		if (*p == ':') {
			// parse port
			hostname = std::string(afterUser, p - afterUser);
			++p;
			afterUser = p;
			for (;;) {
				if (*p == '\0') {
					const std::string buf(afterUser, p - afterUser);
					port = core::string::toInt(buf);
					*strPtr = p;
					return;
				} else if (*p == '/') {
					const std::string buf(afterUser, p - afterUser);
					port = core::string::toInt(buf);
					break;
				}
				++p;
			}
			break;
		} else if (*p == '\0') {
			hostname = std::string(afterUser, p - afterUser);
			*strPtr = p;
			return;
		} else if (*p == '/') {
			hostname = std::string(afterUser, p - afterUser);
			break;
		}
		++p;
	}

	if (*p != '/') {
		_valid = false;
		*strPtr = p;
		return;
	}
	++p;
	*strPtr = p;
}

void Url::parsePath(char **strPtr) {
	char *pos = *strPtr;
	char *p = pos;
	while (*p && *p != '#' && *p != '?') {
		++p;
	}

	path = std::string(pos, p - pos);
	*strPtr += path.size();
	path = "/" + path;
}

void Url::parseQuery(char **strPtr) {
	char *pos = *strPtr;
	if (*pos != '?') {
		return;
	}
	char *p = ++pos;
	while (*p && *p != '#') {
		++p;
	}
	query = std::string(pos, p - pos);
	*strPtr += query.size();
}

void Url::parseFragment(char **strPtr) {
	char *pos = *strPtr;
	if (*pos != '#') {
		return;
	}
	++pos;
	char *p = pos;
	while (*p) p++;
	fragment = std::string(pos, p - pos);
	*strPtr += fragment.size();
}

Url::Url(const char *urlParam) :
		url(core::string::toLower(urlParam)) {
	char *strPtr = (char *)urlParam;
	parseSchema(&strPtr);
	if (!_valid) return;
	parseHostPart(&strPtr);
	if (!_valid) return;
	parsePath(&strPtr);
	if (!_valid) return;
	parseQuery(&strPtr);
	if (!_valid) return;
	parseFragment(&strPtr);
}

Url::~Url() {
}

}