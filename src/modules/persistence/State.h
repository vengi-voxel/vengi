/**
 * @file
 */

#pragma once

#include <string>

#include <libpq-fe.h>
using ResultType = PGresult;

namespace persistence {

class State {
public:
	State(ResultType* res);
	~State();

	State(State&& other);

	State(const State& other) = delete;
	State& operator=(const State& other) = delete;

	ResultType* res = nullptr;

	std::string lastErrorMsg;
	int affectedRows = -1;
	int currentRow = -1;
	// false on error, true on success
	bool result = false;
};

}
