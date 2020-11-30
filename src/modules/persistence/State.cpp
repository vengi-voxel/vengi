/**
 * @file
 */

#include "State.h"
#include "core/Log.h"
#include "core/Assert.h"
#include "core/StringUtil.h"
#include "Connection.h"
#include "postgres/PQSymbol.h"

namespace persistence {

State::State(Connection* connection) :
		_connection(connection) {
}

State::State(State&& other) noexcept :
		res(other.res), lastErrorMsg(other.lastErrorMsg), affectedRows(other.affectedRows),
		cols(other.cols), currentRow(other.currentRow), result(other.result) {
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

bool State::exec(const char *statement, int parameterCount, const char *const *paramValues, const int *paramLengths, const int *paramFormats) {
	core_assert_msg(parameterCount <= 0 || paramValues != nullptr, "Parameters don't match");
	ConnectionType* c = _connection->connection();
#ifdef HAVE_POSTGRES
	if (parameterCount <= 0) {
		res = PQexec(c, statement);
	} else {
		res = PQexecParams(c, statement, parameterCount, nullptr, paramValues, paramLengths, paramFormats, _resultFormat);
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
		_connection->registerPreparedStatement(core::String(name));
	}
	return true;
}

bool State::execPrepared(const char *name, int parameterCount, const char *const *paramValues, const int *paramLengths, const int *paramFormats) {
	ConnectionType* c = _connection->connection();
#ifdef HAVE_POSTGRES
	res = PQexecPrepared(c, name, parameterCount, paramValues, paramLengths, paramFormats, _resultFormat);
#endif
	checkLastResult(c);
	return result;
}

bool State::isBool(const char *value) {
	return *value == '1' || *value == 't' || *value == 'y' || *value == 'o' || *value == 'T';
}

bool State::asBool(int colIndex) const {
	const char *value;
	int length;
	bool isNull;
	getResult(colIndex, FieldType::BOOLEAN, &value, &length, &isNull);
	if (length == 0) {
		return false;
	}
	return isBool(value);
}

int State::asInt(int colIndex) const {
	const char *value;
	int length;
	bool isNull;
	getResult(colIndex, FieldType::INT, &value, &length, &isNull);
	if (length == 0) {
		return 0;
	}
	return core::string::toInt(value);
}

const char *State::columnName(int colIndex) const {
#ifdef HAVE_POSTGRES
	return PQfname(res, colIndex);
#else
	return "";
#endif
}

void State::freeBlob(unsigned char* data) {
	if (data == nullptr) {
		return;
	}
#ifdef HAVE_POSTGRES
	//core_assert(_resultFormat == 0);
	PQfreemem(data);
#endif
}

void State::getResult(int colIndex, FieldType type, const char **value, int *length, bool *isNull) const {
#ifdef HAVE_POSTGRES
	*isNull = PQgetisnull(res, currentRow, colIndex) == 1;
	if (type == FieldType::BLOB) {
		core_assert(_resultFormat == 0);
		const unsigned char *byteArray = (const unsigned char*)PQgetvalue(res, currentRow, colIndex);
		size_t byteArraySize = 0u;
		*value = *isNull ? nullptr : (char*)PQunescapeBytea(byteArray, &byteArraySize);
		*length = *isNull ? 0 : (int)byteArraySize;
	} else {
		*value = *isNull ? nullptr : PQgetvalue(res, currentRow, colIndex);
		*length = PQgetlength(res, currentRow, colIndex);
	}
	if (*value != nullptr) {
		Log::trace("value: %s, length: %i", *value, *length);
	} else {
		Log::trace("value for row %i - col %i is null", currentRow, colIndex);
	}
#else
	*value = nullptr;
#endif
	if (*value == nullptr) {
		*isNull = true;
		*value = "";
		*length = 0;
	}
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
		Log::warn("Non fatal error: %s", lastErrorMsg);
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
