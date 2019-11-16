/**
 * @file
 */

#pragma once

#include "ForwardDecl.h"
#include <string>
#include <unordered_set>

namespace persistence {

class Connection {
private:
	ConnectionType* _connection;
	std::string _host;
	std::string _dbname;
	std::string _user;
	std::string _password;
	uint16_t _port;
	std::unordered_set<std::string> _preparedStatements;
public:
	Connection();
	~Connection();

	bool hasPreparedStatement(const std::string& name) const;
	void registerPreparedStatement(const std::string& name);

	bool status() const;

	void setLoginData(const std::string& username, const std::string& password);

	void changeHost(const std::string& host);

	void changePort(uint16_t port);

	void changeDb(const std::string& dbname);

	void disconnect();

	bool connect();

public:
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
