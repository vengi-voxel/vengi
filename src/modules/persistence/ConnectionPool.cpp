#include "ConnectionPool.h"
#include "core/Log.h"

namespace persistence {

ConnectionPool::ConnectionPool() {
	for (int i = 0; i < _min; ++i) {
		addConnection();
	}
}

ConnectionPool::~ConnectionPool() {
	shutdown();
}

void ConnectionPool::shutdown() {
	while (!_connections.empty()) {
		Connection* c = _connections.back();
		_connections.pop();
		delete c;
	}
	_connectionAmount = 0;
}

void ConnectionPool::addConnection() {
	_connections.push(new Connection());
	++_connectionAmount;
}

void ConnectionPool::giveBack(Connection* c) {
	_connections.push(c);
}

Connection* ConnectionPool::connection() {
	if (_connections.empty()) {
		if (_connectionAmount >= _max) {
			Log::warn("Could not aquire pooled connection, max limit hit");
			return nullptr;
		}
		addConnection();
	}
	Connection* c = _connections.front();
	_connections.pop();
	if (PQstatus(c->connection()) == CONNECTION_OK) {
		return c;
	}
	delete c;
	--_connectionAmount;
	return connection();
}

}
