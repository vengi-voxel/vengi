/**
 * @file
 */

#include "UUID.h"

#include "engine-config.h"

#ifdef HAVE_UUID_H
#include <uuid/uuid.h>
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
#else
	return "";
#endif
}

}
