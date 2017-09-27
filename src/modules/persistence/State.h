/**
 * @file
 */

#pragma once

#include "core/NonCopyable.h"
#include <string>

#include <libpq-fe.h>
using ResultType = PGresult;

namespace persistence {

class State : public core::NonCopyable {
public:
	State(ResultType* res);
	State(State&& other);
	~State();

	ResultType* res = nullptr;

	std::string lastErrorMsg;
	int affectedRows = -1;
	int currentRow = -1;
	// false on error, true on success
	bool result = false;
};

}
