/**
 * @file
 */

#pragma once

#include "ForwardDecl.h"
#include "core/NonCopyable.h"
#include <string>

namespace persistence {

class State : public core::NonCopyable {
public:
	constexpr State() {}
	State(ResultType* res);
	State(State&& other);
	~State();

	ResultType* res = nullptr;

	bool checkLastResult(Connection* connection);
	char* lastErrorMsg = nullptr;
	int affectedRows = -1;
	int currentRow = -1;
	// false on error, true on success
	bool result = false;
};

}
