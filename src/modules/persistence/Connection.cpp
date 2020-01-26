/**
 * @file
 */

#include "Connection.h"
#include "core/Log.h"
#include "postgres/PQSymbol.h"

namespace persistence {

Connection::Connection() :
		_connection(nullptr), _port(0) {
}

Connection::~Connection() {
}

void Connection::setLoginData(const core::String& username, const core::String& password) {
	_password = password;
	_user = username;
}

bool Connection::status() const {
	if (_connection == nullptr) {
		return false;
	}
#ifdef HAVE_POSTGRES
	return PQstatus(_connection) == CONNECTION_OK;
#else
	return false;
#endif
}

void Connection::changeDb(const core::String& dbname) {
	_dbname = dbname;
}

void Connection::changeHost(const core::String& host) {
	_host = host;
}

void Connection::changePort(uint16_t port) {
	_port = port;
}

#ifdef HAVE_POSTGRES
static void defaultNoticeProcessor(void *arg, const char *message) {
	Log::debug("Notice processor: '%s'", message);
}
#endif

bool Connection::connect() {
	if (status()) {
		return true;
	}
#ifdef HAVE_POSTGRES
	core::String conninfo;

	const char *host = nullptr;
	if (!_host.empty()) {
		host = _host.c_str();
	}

	const char *dbname = nullptr;
	if (!_dbname.empty()) {
		dbname = _dbname.c_str();
	}

	const char *user = nullptr;
	if (!_user.empty()) {
		user = _user.c_str();
	}

	const char *password = nullptr;
	if (!_password.empty()) {
		password = _password.c_str();
	}

	core::String port;
	if (_port > 0) {
		port = core::string::toString(_port);
	}

#if 0
	const char *pw = PQencryptPassword(password, user);
	// TODO: do I have to free the result?
#endif
	PQinitSSL(1);
	_connection = PQsetdbLogin(host, port.empty() ? nullptr : port.c_str(), conninfo.c_str(), nullptr, dbname, user, password);
	Log::debug("Database connection %p", _connection);
#else
	_connection = nullptr;
	Log::warn("No postgres support compiled in");
#endif
	if (!status()) {
#ifdef HAVE_POSTGRES
		Log::error("Connection to database failed: '%s'", PQerrorMessage(_connection));
#endif
		disconnect();
		return false;
	}

	_preparedStatements.clear();

#ifdef HAVE_POSTGRES
	if (!PQsslInUse(_connection)) {
		Log::warn("SSL connection to the database failed");
	}
	PQsetNoticeProcessor(_connection, defaultNoticeProcessor, nullptr);
	PGresult *res = PQexec(_connection, "SET TIME ZONE 'UTC';CREATE EXTENSION IF NOT EXISTS pgcrypto;");
	if (PQresultStatus(res) == PGRES_NONFATAL_ERROR || PQresultStatus(res) == PGRES_FATAL_ERROR) {
		Log::error("Failed to set timezone");
	}
	PQclear(res);
#endif
	return true;
}

void Connection::disconnect() {
	if (_connection != nullptr) {
		Log::debug("Disconnect %p", _connection);
#ifdef HAVE_POSTGRES
		PQflush(_connection);
		PQfinish(_connection);
#endif
		_connection = nullptr;
	}
	_preparedStatements.clear();
}

}
