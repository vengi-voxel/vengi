/**
 * @file
 */

#pragma once

#include <string>
#include <unordered_set>
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
	friend class Model;
private:
	ConnectionType* _connection;
	std::string _host;
	std::string _dbname;
	std::string _user;
	std::string _password;
	uint16_t _port;
	std::unordered_set<std::string> _preparedStatements;
	Connection();

	~Connection();

	bool hasPreparedStatement(const std::string& name) const;
	void registerPreparedStatement(const std::string& name);

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

inline bool Connection::hasPreparedStatement(const std::string& name) const {
	return _preparedStatements.find(name) != _preparedStatements.end();
}

inline void Connection::registerPreparedStatement(const std::string& name) {
	_preparedStatements.insert(name);
}

}
