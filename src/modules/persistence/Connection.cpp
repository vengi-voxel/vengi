/**
 * @file
 */

#include "Connection.h"
#include "ConnectionPool.h"
#include "core/Log.h"
#include "config.h"

namespace persistence {

Connection::Connection() :
		_connection(nullptr), _port(0) {
}

Connection::~Connection() {
	disconnect();
}

void Connection::setLoginData(const std::string& username, const std::string& password) {
	_password = password;
	_user = username;
}

void Connection::changeDb(const std::string& dbname) {
	_dbname = dbname;
}

void Connection::changeHost(const std::string& host) {
	_host = host;
}

void Connection::changePort(uint16_t port) {
	_port = port;
}

bool Connection::connect() {
#ifdef PERSISTENCE_POSTGRES
	std::string conninfo = "";

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

	std::string port;
	if (_port > 0) {
		port = std::to_string(_port);
	}

	_connection = PQsetdbLogin(host, port.empty() ? nullptr : port.c_str(), conninfo.c_str(), nullptr, dbname, user, password);
	if (PQstatus(_connection) != CONNECTION_OK) {
		Log::error("Connection to database failed: %s", PQerrorMessage(_connection));
		disconnect();
		return false;
	}
#elif defined PERSISTENCE_SQLITE
	const int rc = sqlite3_open(_dbname.c_str(), &_connection);
	if (rc != SQLITE_OK) {
		Log::error("Can't open database '%s': %s", _dbname.c_str(), sqlite3_errmsg(_connection));
		sqlite3_close(_connection);
		_connection = nullptr;
		return false;
	}
#elif defined PERSISTENCE_MYSQL
	return false;
#endif
	return true;
}

void Connection::disconnect() {
#ifdef PERSISTENCE_POSTGRES
	PQfinish(_connection);
#elif defined PERSISTENCE_SQLITE
	sqlite3_close(_connection);
#endif
	_connection = nullptr;
}

void Connection::close() {
	ConnectionPool::get().giveBack(this);
}

}
