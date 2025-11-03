/**
 * @file
 */

#include "NetworkError.h"
#ifdef WIN32
#include <windows.h>
#include <stdio.h>
#endif
#include <errno.h>
#include <string.h>

namespace network {

const char *getNetworkErrorString() {
#ifdef WIN32
	char buf[256];
	DWORD errCode = GetLastError();
	DWORD len = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errCode,
							   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, sizeof(buf), nullptr);
	if (len == 0) {
		return nullptr;
	}
	static char out[320];
	snprintf(out, sizeof(out), "Win32 error %lu: %s", (unsigned long)errCode, buf);
	return out;
#else
	return strerror(errno);
#endif
}

} // namespace network
