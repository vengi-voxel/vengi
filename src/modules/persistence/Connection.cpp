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

static void defaultNoticeProcessor(void *arg, const char *message) {
	Log::debug("notice processor: %s", message);
}

bool Connection::connect() {
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
	Log::debug("connect %p", _connection);
	if (PQstatus(_connection) != CONNECTION_OK) {
		Log::error("Connection to database failed: %s", PQerrorMessage(_connection));
		disconnect();
		return false;
	}

	_preparedStatements.clear();

	PQsetNoticeProcessor(_connection, defaultNoticeProcessor, nullptr);
	return true;
}

void Connection::disconnect() {
	if (_connection != nullptr) {
		Log::debug("disconnect %p", _connection);
		PQfinish(_connection);
		_connection = nullptr;
	}
	_preparedStatements.clear();
}

void Connection::close() {
	ConnectionPool::get().giveBack(this);
}

}
