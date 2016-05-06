/**
 * @file
 */

#include "Connection.h"
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

inline std::string Connection::escape(const std::string& value) const {
	// TODO: escape ' inside the value
	return "'" + value + "'";
}

bool Connection::connect() {
	std::string conninfo = "connect_timeout='2'";

	if (!_host.empty())
		conninfo += " host=" + escape(_host);
	if (!_dbname.empty())
		conninfo += " dbname=" + escape(_dbname);
	if (!_user.empty())
		conninfo += " user=" + escape(_user);
	if (!_password.empty())
		conninfo += " password=" + escape(_password);
	if (_port > 0)
		conninfo += " port=" + escape(std::to_string(_port));

	_pgConnection = PQconnectdb(conninfo.c_str());

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

}
