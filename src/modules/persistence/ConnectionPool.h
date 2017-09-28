/**
 * @file
 */

#pragma once

#include "Connection.h"
#include "core/Var.h"
#include <queue>

namespace persistence {

/**
 * One connection pool per thread
 */
class ConnectionPool {
	friend class Connection;
protected:
	int _min = -1;
	int _max = -1;
	int _connectionAmount = 0;
	core::VarPtr _dbName;
	core::VarPtr _dbHost;
	core::VarPtr _dbUser;
	core::VarPtr _dbPw;

	std::queue<Connection*> _connections;

public:
	ConnectionPool();
	~ConnectionPool();

	int init();
	void shutdown();

	/**
	 * @brief Gets one connection from the pool
	 * @note Make sure to call @c Connection::close() to give the connection back to the pool.
	 * @return @c Connection object
	 */
	Connection* connection();

private:
	Connection* addConnection();
	void giveBack(Connection* c);
};

}
