/**
 * @file
 */

#include "UUID.h"

#include "engine-config.h"
#include <SDL_platform.h>

#ifdef HAVE_UUID_H
#include <uuid/uuid.h>
#elif __WINDOWS__
#include <windows.h>
#elif __APPLE__
#include <CoreFoundation/CFUUID.h>
#else
#warning "No uuid implementation found"
#endif

namespace core {

core::String generateUUID() {
#ifdef HAVE_UUID_H
	uuid_t uuid;
	uuid_generate_random(uuid);
	char buf[37];
	uuid_unparse(uuid, buf);
	return std::string(buf);
#elif __WINDOWS__
	UUID uuid;
	UuidCreate(&uuid);
	char *str;
	UuidToStringA(&uuid, (RPC_CSTR*)&str);
	core::String uuidStr = str;
	RpcStringFreeA((RPC_CSTR*)&str);
	return uuidStr;
#elif __APPLE__
	auto newId = CFUUIDCreate(nullptr);
	auto bytes = CFUUIDGetUUIDBytes(newId);
	CFRelease(newId);
	const core::Array<unsigned char, 16> arr =
	{{
		bytes.byte0,
		bytes.byte1,
		bytes.byte2,
		bytes.byte3,
		bytes.byte4,
		bytes.byte5,
		bytes.byte6,
		bytes.byte7,
		bytes.byte8,
		bytes.byte9,
		bytes.byte10,
		bytes.byte11,
		bytes.byte12,
		bytes.byte13,
		bytes.byte14,
		bytes.byte15
	}};
	return std::string(std::begin(arr), std::end(arr));
#else
	return "";
#endif
}

}
