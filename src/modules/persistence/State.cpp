/**
 * @file
 */

#include "State.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "Connection.h"
#include "engine-config.h"
#ifdef HAVE_POSTGRES
#include <libpq-fe.h>
#endif

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
#ifdef HAVE_POSTGRES
		PQclear(res);
#endif
		res = nullptr;
	}
	lastErrorMsg = nullptr;
}

bool State::exec(const char *statement, int parameterCount, const char *const *paramValues) {
	core_assert_msg(parameterCount <= 0 || paramValues != nullptr, "Parameters don't match");
	ConnectionType* c = _connection->connection();
#ifdef HAVE_POSTGRES
	if (parameterCount <= 0) {
		res = PQexec(c, statement);
	} else {
		res = PQexecParams(c, statement, parameterCount, nullptr, paramValues, nullptr, nullptr, 0);
	}
#endif
	checkLastResult(c);
	return result;
}

bool State::prepare(const char *name, const char* statement, int parameterCount) {
	ConnectionType* c = _connection->connection();
#ifdef HAVE_POSTGRES
	res = PQprepare(c, name, statement, parameterCount, nullptr);
#endif
	checkLastResult(c);
	if (!result) {
		return false;
	}
	if (name != nullptr && name[0] != '\0') {
		_connection->registerPreparedStatement(std::string(name));
	}
	return true;
}

bool State::execPrepared(const char *name, int parameterCount, const char *const *paramValues) {
	ConnectionType* c = _connection->connection();
#ifdef HAVE_POSTGRES
	res = PQexecPrepared(c, name, parameterCount, paramValues, nullptr, nullptr, 0);
#endif
	checkLastResult(c);
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

#ifdef HAVE_POSTGRES
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
		cols = PQnfields(res);
		currentRow = 0;
		result = true;
		Log::debug("Affected rows %i", affectedRows);
		break;
	default:
		Log::error("Unknown state: %s", PQresStatus(lastState));
		break;
	}
#endif
}

}
