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
	Connection* _connection = nullptr;
	void checkLastResult(ConnectionType* connection);
public:
	constexpr State() {
	}

	State(Connection* connection);
	State(State&& other);
	~State();

	bool exec(const char* statement);
	bool prepare(const char *name, const char* statement, int parameterCount);
	bool execPrepared(const char *name, int parameterCount, const char *const *paramValues);

	ResultType* res = nullptr;

	char* lastErrorMsg = nullptr;
	int affectedRows = -1;
	int currentRow = -1;
	// false on error, true on success
	bool result = false;
};

}
