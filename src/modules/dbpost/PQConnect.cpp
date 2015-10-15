#include "PQConnect.h"
#include "core/Log.h"

namespace dbpost {

PQConnect::PQConnect() :
		_pgConnection(nullptr) {
}

PQConnect::~PQConnect() {
	disconnect();
}

void PQConnect::setLoginData(const std::string& username, const std::string& password) {
	_password = password;
	_user = username;
}

void PQConnect::changeDb(const std::string& dbname) {
	_dbname = dbname;
}

void PQConnect::changeHost(const std::string& host) {
	_host = host;
}

void PQConnect::changePort(const std::string& port) {
	_port = port;
}

int PQConnect::connect() {
	//const std::string conninfo = "hostaddr=" + _host + " port=" + _port + " dbname=" + _dbname + " user=" + _user + " password=" + _password;
	std::string conninfo = "";

	if (!_host.empty())
		conninfo += " host=" + _host;
	if (!_dbname.empty())
		conninfo += " dbname=" + _dbname;
	if (!_user.empty())
		conninfo += " user=" + _user;
	if (!_password.empty())
		conninfo += " password=" + _password;
	if (!_port.empty())
		conninfo += " port=" + _port;

	Log::info("Connection: %s", conninfo.c_str());

	_pgConnection = PQconnectdb(conninfo.c_str());

	if (PQstatus(_pgConnection) != CONNECTION_OK) {
		Log::error("Connection to database failed: %s", PQerrorMessage(_pgConnection));
		disconnect();
		return -1;
	}

	return 0;
}

void PQConnect::disconnect() {
	PQfinish(_pgConnection);
	_pgConnection = nullptr;
}

}
