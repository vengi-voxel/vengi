/**
 * @file
 */

#include "Model.h"
#include "ConnectionPool.h"
#include "ScopedConnection.h"
#include "ConstraintType.h"
#include "core/Log.h"
#include "core/String.h"
#include <algorithm>

// TODO: remove me
#include <libpq-fe.h>

namespace persistence {

Model::Model(const std::string& tableName) :
		_tableName(tableName) {
	_membersPointer = (uint8_t*)this;
}

Model::~Model() {
	_fields.clear();
}

bool Model::isPrimaryKey(const std::string& fieldname) const {
	auto i = std::find_if(_fields.begin(), _fields.end(),
			[&fieldname](const Field& f) {return f.name == fieldname;}
	);
	if (i == _fields.end()) {
		return false;
	}

	return (i->contraintMask & std::enum_value(ConstraintType::PRIMARYKEY)) != 0;
}

PreparedStatement Model::prepare(const std::string& name, const std::string& statement) {
	return PreparedStatement(this, name, statement);
}

bool Model::checkLastResult(State& state, Connection* connection) const {
	state.affectedRows = 0;
	if (state.res == nullptr) {
		Log::debug("Empty result");
		return false;
	}

	ExecStatusType lastState = PQresultStatus(state.res);

	switch (lastState) {
	case PGRES_NONFATAL_ERROR:
		state.lastErrorMsg = PQerrorMessage(connection->connection());
		Log::warn("non fatal error: %s", state.lastErrorMsg);
		break;
	case PGRES_BAD_RESPONSE:
	case PGRES_FATAL_ERROR:
		state.lastErrorMsg = PQerrorMessage(connection->connection());
		Log::error("fatal error: %s", state.lastErrorMsg);
		PQclear(state.res);
		state.res = nullptr;
		return false;
	case PGRES_EMPTY_QUERY:
	case PGRES_COMMAND_OK:
	case PGRES_TUPLES_OK:
		state.affectedRows = PQntuples(state.res);
		state.currentRow = 0;
		Log::debug("Affected rows %i", state.affectedRows);
		break;
	default:
		Log::error("unknown state: %s", PQresStatus(lastState));
		return false;
	}

	state.result = true;
	return true;
}

bool Model::exec(const char* query) {
	Log::debug("%s", query);
	ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
	if (!scoped) {
		Log::error("Could not execute query '%s' - could not acquire connection", query);
		return false;
	}
	ConnectionType* conn = scoped.connection()->connection();
	State s(PQexec(conn, query));
	checkLastResult(s, scoped);
	if (s.affectedRows > 1) {
		Log::debug("More than one row affected, can't fill model values");
		return s.result;
	} else if (s.affectedRows <= 0) {
		Log::trace("No rows affected, can't fill model values");
		return s.result;
	}
	return fillModelValues(s);
}

bool Model::exec(const char* query) const {
	Log::debug("%s", query);
	ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
	if (!scoped) {
		Log::error("Could not execute query '%s' - could not acquire connection", query);
		return false;
	}
	ConnectionType* conn = scoped.connection()->connection();
	State s(PQexec(conn, query));
	return checkLastResult(s, scoped);
}

const Field& Model::getField(const std::string& name) const {
	for (const Field& field : _fields) {
		if (field.name == name) {
			return field;
		}
	}
	static const Field emptyField;
	return emptyField;
}

bool Model::begin() {
	return exec("START TRANSACTION;");
}

bool Model::commit() {
	return exec("COMMIT;");
}

bool Model::rollback() {
	return exec("ROLLBACK;");
}

bool Model::fillModelValues(State& state) {
	const int cols = PQnfields(state.res);
	Log::trace("Query has values for %i cols", cols);
	for (int i = 0; i < cols; ++i) {
		const char* name = PQfname(state.res, i);
		const Field& f = getField(name);
		if (f.name != name) {
			Log::error("Unknown field name for '%s'", name);
			state.result = false;
			return false;
		}
		const bool isNull = PQgetisnull(state.res, state.currentRow, i);
		const char* value = isNull ? nullptr : PQgetvalue(state.res, state.currentRow, i);
		int length = PQgetlength(state.res, state.currentRow, i);
		if (value == nullptr) {
			value = "";
			length = 0;
		}
		Log::debug("Try to set '%s' to '%s' (length: %i)", name, value, length);
		switch (f.type) {
		case FieldType::TEXT:
		case FieldType::STRING:
		case FieldType::PASSWORD:
			setValue(f, std::string(value, length));
			break;
		case FieldType::INT:
			setValue(f, core::string::toInt(value));
			break;
		case FieldType::LONG:
			setValue(f, core::string::toLong(value));
			break;
		case FieldType::TIMESTAMP: {
			setValue(f, Timestamp(core::string::toLong(value)));
			break;
		}
		case FieldType::MAX:
			break;
		}
		setIsNull(f, isNull);
	}
	return true;
}

}
