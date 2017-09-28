/**
 * @file
 */

#include "DBHandler.h"
#include "core/Singleton.h"
#include "ConnectionPool.h"
#include "core/Log.h"
#include <sstream>

namespace persistence {

DBHandler::DBHandler() {
}

bool DBHandler::init() {
	if (core::Singleton<ConnectionPool>::getInstance().init() <= 0) {
		Log::error("Failed to init the connection pool");
		return false;
	}
	return true;
}

void DBHandler::shutdown() {
	core::Singleton<ConnectionPool>::getInstance().shutdown();
}

bool DBHandler::truncate(const Model& model) const {
	return exec(createTruncateTableStatement(model));
}

bool DBHandler::truncate(Model&& model) const {
	return exec(createTruncateTableStatement(model));
}

bool DBHandler::createTable(Model&& model) const {
	return exec(createCreateTableStatement(model));
}

bool DBHandler::exec(const std::string& query) const {
	return execInternal(query).result;
}

State DBHandler::execInternal(const std::string& query) const {
	ScopedConnection scoped(core::Singleton<ConnectionPool>::getInstance().connection());
	if (!scoped) {
		Log::error("Could not execute query '%s' - could not acquire connection", query.c_str());
		return State();
	}
	State s(scoped.connection());
	if (!s.exec(query.c_str())) {
		Log::warn("Failed to execute query: '%s'", query.c_str());
	} else {
		Log::debug("Executed query: '%s'", query.c_str());
	}
	return s;
}

bool DBHandler::begin() {
	return exec(createTransactionBegin());
}

bool DBHandler::commit() {
	return exec(createTransactionCommit());
}

bool DBHandler::rollback() {
	return exec(createTransactionRollback());
}

}
