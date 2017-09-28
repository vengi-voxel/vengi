/**
 * @file
 */

#pragma once

#include "ForwardDecl.h"
#include "core/NonCopyable.h"
#include <string>

namespace persistence {

class State : public core::NonCopyable {
private:
	void checkLastResult(ConnectionType* connection);
public:
	constexpr State() {
	}
	State(ConnectionType* connection, ResultType* res);
	State(State&& other);
	~State();

	ResultType* res = nullptr;

	char* lastErrorMsg = nullptr;
	int affectedRows = -1;
	int currentRow = -1;
	// false on error, true on success
	bool result = false;
};

}
