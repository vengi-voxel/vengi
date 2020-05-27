/**
 * @file
 */

#pragma once

#include "ForwardDecl.h"
#include "core/String.h"
#include <unordered_set>

namespace persistence {

class Connection {
private:
	ConnectionType* _connection = nullptr;
	core::String _host;
	core::String _dbname;
	core::String _user;
	core::String _password;
	uint16_t _port = 0u;
	std::unordered_set<core::String, core::StringHash> _preparedStatements;
public:
	bool hasPreparedStatement(const core::String& name) const;
	void registerPreparedStatement(const core::String& name);

	bool status() const;

	void setLoginData(const core::String& username, const core::String& password);

	void changeHost(const core::String& host);

	void changePort(uint16_t port);

	void changeDb(const core::String& dbname);

	void disconnect();

	bool connect();

public:
	ConnectionType* connection() const;
};

inline ConnectionType* Connection::connection() const {
	return _connection;
}

inline bool Connection::hasPreparedStatement(const core::String& name) const {
	return _preparedStatements.find(name) != _preparedStatements.end();
}

inline void Connection::registerPreparedStatement(const core::String& name) {
	_preparedStatements.insert(name);
}

}
