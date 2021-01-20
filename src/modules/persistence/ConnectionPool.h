/**
 * @file
 */

#pragma once

#include "Connection.h"
#include "core/Var.h"
#include "core/Trace.h"
#include "core/collection/ConcurrentPriorityQueue.h"
#include "core/IComponent.h"
#include "core/concurrent/Atomic.h"

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
	core::AtomicInt _connectionAmount { 0 };
	core::VarPtr _dbName;
	core::VarPtr _dbHost;
	core::VarPtr _dbPort;
	core::VarPtr _dbUser;
	core::VarPtr _dbPw;
	core::VarPtr _minConnections;
	core::VarPtr _maxConnections;

	core::ConcurrentPriorityQueue<Connection*> _connections;

public:
	ConnectionPool();
	~ConnectionPool();

	bool init() override;
	void shutdown() override;

	/**
	 * @brief Gets one connection from the pool
	 * @note Make sure to call @c giveBack() to give the connection back to the pool.
	 * @return @c Connection object
	 * @sa ScopedConnection
	 */
	Connection* connection();
	void giveBack(Connection* c);

	int connections() const;

private:
	Connection* addConnection();
};

inline int ConnectionPool::connections() const {
	return _connectionAmount;
}

}
