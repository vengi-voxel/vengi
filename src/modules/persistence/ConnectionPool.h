#pragma once

#include <queue>
#include "Connection.h"
#include "ScopedConnection.h"
#include "core/Var.h"
#include "core/Singleton.h"

namespace persistence {

/**
 * One connection pool per thread
 */
class ConnectionPool {
	friend class Connection;
	friend class core::Singleton<ConnectionPool>;
protected:
	int _min = -1;
	int _max = -1;
	int _connectionAmount = 0;
	core::VarPtr _dbName;
	core::VarPtr _dbHost;
	core::VarPtr _dbUser;
	core::VarPtr _dbPw;

	std::queue<Connection*> _connections;

	ConnectionPool();
public:
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
