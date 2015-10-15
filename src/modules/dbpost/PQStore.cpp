#include "PQStore.h"
#include "StoreInterface.h"
#include "core/Log.h"
#include <sstream>

namespace dbpost {

PQStore::PQStore(PQConnect* conn) :
		_connection(conn), _usable(true), _res(nullptr), _lastState(PGRES_EMPTY_QUERY), _affectedRows(0) {
}

void PQStore::storeModel(StoreInterface& model) {
	const std::string& insertSql = sqlBuilder(model, false);

	//trBegin();
	query(insertSql);
	//trEnd();
}

void PQStore::createNeeds(const StoreInterface& model) {
	const std::string& crSql = model.getCreate();
	query(crSql);
}

std::unordered_map<std::string, std::string> PQStore::loadModel(const StoreInterface& model) {
	const std::string& loadSql = sqlLoadBuilder(model, false);
	Log::trace("sql used %s", loadSql.c_str());
	std::unordered_map<std::string, std::string> dbResult;
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

std::string PQStore::sqlBuilder(const StoreInterface& model, bool update) const {
	std::stringstream insertSql;
	insertSql << "INSERT INTO " << model.getTableName() << " ";
	std::stringstream fieldKeys;
	std::stringstream valueKeys;

	const std::unordered_map<std::string, std::string>& fields = model.getFields();

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

std::string PQStore::sqlLoadBuilder(const StoreInterface& model, bool update) const {
	std::string loadSql = "SELECT * FROM " + model.getTableName() + " ";
	std::string fieldKeys = "";

	const std::unordered_map<std::string, std::string>& fields = model.getFields();

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

void PQStore::trBegin() {
	if (!_usable)
		return;

	_res = PQexec(_connection->connection(), "BEGIN");
	if (checkLastResult()) {
		// anyway ..res no longer needed but already closed in case of fail
		PQclear(_res);
	}
}

void PQStore::trEnd() {
	if (!_usable)
		return;

	_res = PQexec(_connection->connection(), "END");
	if (checkLastResult()) {
		// anyway ..res no longer needed but already closed in case of fail
		PQclear(_res);
	}
}

bool PQStore::checkLastResult() {
	_affectedRows = 0;
	Log::info("get result");
	if (_res != nullptr)
		_lastState = PQresultStatus(_res);
	else
		return false;

	if ((_lastState == PGRES_EMPTY_QUERY) || (_lastState == PGRES_BAD_RESPONSE) || (_lastState == PGRES_FATAL_ERROR)) {
		PQclear(_res);
		char* msg = PQerrorMessage(_connection->connection());
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
		Log::info("Data read %i", _affectedRows);
		return true;
	}

	Log::error("not catched state: %s", PQresStatus(_lastState));
	// what else ?
	return false;
}

bool PQStore::query(const std::string& query) {
	if (_usable) {
		Log::trace("SEND: %s", query.c_str());
		_res = PQexec(_connection->connection(), query.c_str());
		return checkLastResult();
	}
	Log::error("DB Error: connection not usable");
	return false;
}

PQStore::~PQStore() {
	// TODO: assigning a nullptr is not possible for a reference
	//_connection = nullptr;
}

}
