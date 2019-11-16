/**
 * @file
 */

#pragma once

#include "Connection.h"
#include "core/Var.h"
#include "core/Trace.h"
#include "core/IComponent.h"
#include <queue>
#include <mutex>

namespace persistence {

/**
 * One connection pool per thread
 */
class ConnectionPool : public core::IComponent {
	friend class Connection;
	friend class ScopedConnection;
protected:
	int _min = -1;
	int _max = -1;
	int _connectionAmount = 0;
	core::VarPtr _dbName;
	core::VarPtr _dbHost;
	core::VarPtr _dbUser;
	core::VarPtr _dbPw;
	core::VarPtr _minConnections;
	core::VarPtr _maxConnections;
	mutable core_trace_mutex(std::mutex, _mutex);

	std::queue<Connection*> _connections;

public:
	ConnectionPool();
	~ConnectionPool();

	bool init() override;
	void shutdown() override;

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
