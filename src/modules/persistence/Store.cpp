#include "Store.h"
#include "core/Log.h"
#include <sstream>

namespace persistence {

// TODO: this is far away from being thread safe and also fails hard if the user table is not available.

Store::Store(Connection* conn) :
		_connection(conn), _usable(true), _res(nullptr), _lastState(PGRES_EMPTY_QUERY), _affectedRows(0) {
}

bool Store::storeModel(PeristenceModel& model) {
	const std::string& insertSql = sqlBuilder(model, false);
	return query(insertSql);
}

bool Store::createNeeds(const PeristenceModel& model) {
	const std::string& crSql = model.getCreate();
	return query(crSql);
}

KeyValueMap Store::loadModel(const PeristenceModel& model) {
	const std::string& loadSql = sqlLoadBuilder(model, false);
	Log::trace("sql used %s", loadSql.c_str());
	KeyValueMap dbResult;
	if (query(loadSql) && _affectedRows == 1) {
		const int nFields = PQnfields(_res);
		for (int i = 0; i < nFields; ++i) {
			const char* name = PQfname(_res, i);
			const char* value = PQgetvalue(_res, 0, i);
			dbResult[std::string(name)] = std::string(value);
			//model.update(tname, fvalue);
		}
	}
	PQclear(_res);

	return dbResult;
}

std::string Store::sqlBuilder(const PeristenceModel& model, bool update) const {
	std::stringstream insertSql;
	insertSql << "INSERT INTO " << model.getTableName() << " ";
	std::stringstream fieldKeys;
	std::stringstream valueKeys;

	const Fields& fields = model.getFields();

	std::string add = "";
	for (auto p = fields.begin(); p != fields.end(); ++p) {
		const std::string& strKey = p->first;
		const std::string& strValue = p->second;
		if (!model.isSerial(strKey)) {
			fieldKeys << add << strKey;
			valueKeys << add << "'" << strValue << "'";
			add = ", ";
		}
	}

	insertSql << "(" << fieldKeys.str() << ") VALUES (" << valueKeys.str() << ");";
	const std::string& str = insertSql.str();
	Log::trace("used query %s", str.c_str());
	return str;
}

std::string Store::sqlLoadBuilder(const PeristenceModel& model, bool update) const {
	std::string loadSql = "SELECT * FROM " + model.getTableName() + " ";
	std::string fieldKeys = "";

	const Fields& fields = model.getFields();

	std::string add = "";
	for (auto p = fields.begin(); p != fields.end(); ++p) {
		const std::string& strKey = p->first;
		const std::string& strValue = p->second;
		if (!model.isSerial(strKey)) {
			fieldKeys += add + strKey + " = '" + strValue + "'";
			add = " AND ";
		}
	}

	loadSql += "WHERE " + fieldKeys + ";";
	return loadSql;
}

void Store::trBegin() {
	if (!_usable)
		return;

	_res = PQexec(_connection->connection(), "BEGIN");
	if (checkLastResult()) {
		// anyway ..res no longer needed but already closed in case of fail
		PQclear(_res);
	}
}

void Store::trEnd() {
	if (!_usable)
		return;

	_res = PQexec(_connection->connection(), "END");
	if (checkLastResult()) {
		// anyway ..res no longer needed but already closed in case of fail
		PQclear(_res);
	}
}

bool Store::checkLastResult() {
	_affectedRows = 0;
	if (_res == nullptr)
		return false;

	_lastState = PQresultStatus(_res);

	if ((_lastState == PGRES_EMPTY_QUERY) || (_lastState == PGRES_BAD_RESPONSE) || (_lastState == PGRES_FATAL_ERROR)) {
		PQclear(_res);
		const char* msg = PQerrorMessage(_connection->connection());
		_lastErrorMsg = std::string(msg);

		Log::error("Failed to execute sql: %s ", _lastErrorMsg.c_str());
		return false;
	}

	if (_lastState == PGRES_COMMAND_OK) {
		// no data in return but all fine
		PQclear(_res);
		_affectedRows = 0;
		return true;
	}

	if (_lastState == PGRES_TUPLES_OK) {
		_affectedRows = PQntuples(_res);
		Log::trace("Affected rows on read %i", _affectedRows);
		return true;
	}

	Log::error("not catched state: %s", PQresStatus(_lastState));
	// what else ?
	return false;
}

bool Store::query(const std::string& query) {
	if (_usable) {
		Log::trace("SEND: %s", query.c_str());
		_res = PQexec(_connection->connection(), query.c_str());
		return checkLastResult();
	}
	Log::error("DB Error: connection not usable");
	return false;
}

Store::~Store() {
	// TODO: assigning a nullptr is not possible for a reference
	//_connection = nullptr;
}

}
