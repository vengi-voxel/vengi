#pragma once

#include "sqlite3.h"
#include "PolyVox/ErrorHandling.h"
#include "core/Common.h"

namespace Cubiquity {

#define EXECUTE_SQLITE_FUNC(function) \
	do \
	{ \
		int rc = function; \
		core_assert_msg(rc == SQLITE_OK, "Encountered '%s' (error code %i) when executing '%s'", sqlite3_errstr(rc), rc, CORE_STRINGIFY_INTERNAL(function)); \
	} while (0)

}
