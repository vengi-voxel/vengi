/**
 * @file
 */

#include "State.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "Connection.h"
#include <libpq-fe.h>

namespace persistence {

State::State(Connection* connection) :
		_connection(connection) {
}

State::State(State&& other) :
		res(other.res), lastErrorMsg(other.lastErrorMsg), affectedRows(
				other.affectedRows), result(other.result) {
	other.res = nullptr;
	other._connection = nullptr;
	other.lastErrorMsg = nullptr;
}

State::~State() {
	if (res != nullptr) {
		PQclear(res);
		res = nullptr;
	}
	lastErrorMsg = nullptr;
}

bool State::exec(const char *statement, int parameterCount, const char *const *paramValues) {
	core_assert_msg(parameterCount <= 0 || paramValues != nullptr, "Parameters don't match");
	ConnectionType* c = _connection->connection();
	if (parameterCount <= 0) {
		res = PQexec(c, statement);
	} else {
		res = PQexecParams(c, statement, parameterCount, nullptr, paramValues, nullptr, nullptr, 0);
	}
	checkLastResult(c);
	return result;
}

bool State::prepare(const char *name, const char* statement, int parameterCount) {
	res = PQprepare(_connection->connection(), name, statement, parameterCount, nullptr);
	checkLastResult(_connection->connection());
	if (!result) {
		return false;
	}
	if (name != nullptr && name[0] != '\0') {
		_connection->registerPreparedStatement(std::string(name));
	}
	return true;
}

bool State::execPrepared(const char *name, int parameterCount, const char *const *paramValues) {
	res = PQexecPrepared(_connection->connection(), name, parameterCount, paramValues, nullptr, nullptr, 0);
	checkLastResult(_connection->connection());
	return result;
}

void State::checkLastResult(ConnectionType* connection) {
	affectedRows = 0;
	if (res == nullptr) {
		result = true;
		Log::debug("Empty result");
		return;
	}
	result = false;

	ExecStatusType lastState = PQresultStatus(res);

	switch (lastState) {
	case PGRES_NONFATAL_ERROR: {
		char *lastErrorMsg = PQerrorMessage(connection);
		Log::warn("Mon fatal error: %s", lastErrorMsg);
		PQfreemem(lastErrorMsg);
		result = true;
		break;
	}
	case PGRES_BAD_RESPONSE:
	case PGRES_FATAL_ERROR:
		lastErrorMsg = PQerrorMessage(connection);
		Log::error("Fatal error: %s", lastErrorMsg);
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
}

}
