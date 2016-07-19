/**
 * @file
 */

#pragma once

#include <string>
#include "config.h"

#ifdef PERSISTENCE_POSTGRES
#include <libpq-fe.h>
using ConnectionType = ::PGconn;
#elif defined PERSISTENCE_SQLITE
#include <sqlite3.h>
using ConnectionType = ::sqlite3;
#endif

namespace persistence {

class Connection {
	friend class ConnectionPool;
private:
	ConnectionType* _connection;
	std::string _host;
	std::string _dbname;
	std::string _user;
	std::string _password;
	uint16_t _port;
	Connection();

	~Connection();

	void setLoginData(const std::string& username, const std::string& password);

	void changeHost(const std::string& host);

	void changePort(uint16_t port);

	void changeDb(const std::string& dbname);

	void disconnect();

	bool connect();

public:
	void close();

	ConnectionType* connection() const;
};

inline ConnectionType* Connection::connection() const {
	return _connection;
}

}
