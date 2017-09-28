/**
 * @file
 */

#include "State.h"
#include "core/Log.h"
#include "Connection.h"
#include <libpq-fe.h>

namespace persistence {

State::State(ResultType* _res) :
		res(_res) {
}

State::State(State&& other) :
		res(other.res), lastErrorMsg(other.lastErrorMsg), affectedRows(
				other.affectedRows), result(other.result) {
	other.res = nullptr;
	other.lastErrorMsg = nullptr;
}

State::~State() {
	if (res != nullptr) {
		PQclear(res);
		res = nullptr;
	}
	if (lastErrorMsg != nullptr) {
		PQfreemem(lastErrorMsg);
		lastErrorMsg = nullptr;
	}
}

bool State::checkLastResult(Connection* connection) {
	affectedRows = 0;
	result = false;
	if (res == nullptr) {
		Log::debug("Empty result");
		return false;
	}

	ExecStatusType lastState = PQresultStatus(res);

	switch (lastState) {
	case PGRES_NONFATAL_ERROR: {
		char *lastErrorMsg = PQerrorMessage(connection->connection());
		Log::warn("non fatal error: %s", lastErrorMsg);
		PQfreemem(lastErrorMsg);
		result = true;
		// TODO: wtf is a non-fatal error?
		break;
	}
	case PGRES_BAD_RESPONSE:
	case PGRES_FATAL_ERROR:
		lastErrorMsg = PQerrorMessage(connection->connection());
		Log::error("fatal error: %s", lastErrorMsg);
		break;
	case PGRES_EMPTY_QUERY:
	case PGRES_COMMAND_OK:
	case PGRES_TUPLES_OK:
		affectedRows = PQntuples(res);
		currentRow = 0;
		result = true;
		Log::debug("Affected rows %i", affectedRows);
		break;
	default:
		Log::error("Unknown state: %s", PQresStatus(lastState));
		break;
	}

	return result;
}

}
