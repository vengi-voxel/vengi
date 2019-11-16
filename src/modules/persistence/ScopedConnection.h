/**
 * @file
 */

#pragma once

#include "ForwardDecl.h"

namespace persistence {

class ConnectionPool;

class ScopedConnection {
private:
	ConnectionPool& _connectionPool;
	Connection* _c;
public:
	ScopedConnection(ConnectionPool& connectionPool, Connection* c) :
			_connectionPool(connectionPool), _c(c) {
	}

	inline operator Connection*() {
		return _c;
	}

	inline Connection* connection() {
		return _c;
	}

	inline operator bool() const {
		return _c != nullptr;
	}

	~ScopedConnection();
};

}
