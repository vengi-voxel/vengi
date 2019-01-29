/**
 * @file
 */

#include "UUID.h"

#include "engine-config.h"

#ifdef HAVE_UUID_H
#include <uuid/uuid.h>
#elif WIN32
#include <windows.h>
#else
#warning "No uuid implementation found"
#endif

namespace core {

std::string generateUUID() {
#ifdef HAVE_UUID_H
	uuid_t uuid;
	uuid_generate_random(uuid);
	char buf[37];
	uuid_unparse(uuid, buf);
	return std::string(buf);
#elif WIN32
	UUID uuid;
	UuidCreate(&uuid);
	char *str;
	UuidToStringA(&uuid, (RPC_CSTR*)&str);
	std::string uuidStr = str;
	RpcStringFreeA((RPC_CSTR*)&str);
	return uuidStr;
#else
	return "";
#endif
}

}
