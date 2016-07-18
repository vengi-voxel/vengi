/**
 * @file
 */

#include "Connection.h"
#include "ConnectionPool.h"
#include "core/Log.h"

namespace persistence {

Connection::Connection() :
		_pgConnection(nullptr), _port(0) {
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
	std::string conninfo = "connect_timeout='2'";

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

	_pgConnection = PQsetdbLogin(host, port.empty() ? nullptr : port.c_str(), conninfo.c_str(), nullptr, dbname, user, password);
	if (PQstatus(_pgConnection) != CONNECTION_OK) {
		Log::error("Connection to database failed: %s", PQerrorMessage(_pgConnection));
		disconnect();
		return false;
	}

	return true;
}

void Connection::disconnect() {
	PQfinish(_pgConnection);
	_pgConnection = nullptr;
}

void Connection::close() {
	ConnectionPool::get().giveBack(this);
}

}
