#pragma once

#include <queue>
#include "Connection.h"
#include "ScopedConnection.h"

namespace persistence {

/**
 * One connection pool per thread
 */
class ConnectionPool {
	friend class Connection;
protected:
	// TODO: move into cvar
	int _min = 2;
	int _max = 10;
	int _connectionAmount = 0;

	std::queue<Connection*> _connections;

	ConnectionPool();
public:
	~ConnectionPool();

	void shutdown();

	static ConnectionPool& get() {
		thread_local ConnectionPool pool;
		return pool;
	}

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
