#include "ConnectionPool.h"
#include "core/Log.h"
#include "core/Common.h"
#include "core/GameConfig.h"

namespace persistence {

ConnectionPool::ConnectionPool() {
}

ConnectionPool::~ConnectionPool() {
	shutdown();
}

int ConnectionPool::init(const char *password, const char *user, const char *database, const char *host) {
	_min = core::Var::get(cfg::DatabaseMinConnections, "2")->intVal();
	_max = core::Var::get(cfg::DatabaseMaxConnections, "10")->intVal();

	core_assert_always(_min <= _max);

	_dbName = core::Var::get(cfg::DatabaseName, database);
	_dbHost = core::Var::get(cfg::DatabaseHost, host);
	_dbUser = core::Var::get(cfg::DatabaseUser, user);
	_dbPw = core::Var::get(cfg::DatabasePassword, password);

	Log::debug("Connect to %s@%s to database %s", _dbUser->strVal().c_str(), _dbHost->strVal().c_str(), _dbName->strVal().c_str());

	for (int i = 0; i < _min; ++i) {
		addConnection();
	}

	return _connectionAmount;
}

void ConnectionPool::shutdown() {
	while (!_connections.empty()) {
		Connection* c = _connections.front();
		c->disconnect();
		_connections.pop();
		delete c;
	}
	_connectionAmount = 0;

	_dbName = core::VarPtr();
	_dbHost = core::VarPtr();
	_dbUser = core::VarPtr();
	_dbPw = core::VarPtr();
}

Connection* ConnectionPool::addConnection() {
	Connection* c = new Connection();

	c->changeDb(_dbName->strVal());
	c->changeHost(_dbHost->strVal());
	c->setLoginData(_dbUser->strVal(), _dbPw->strVal());

	_connections.push(c);
	++_connectionAmount;
	return c;
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
		Connection* newC = addConnection();
		if (!newC->connect()) {
			Log::error("Could not connect to database");
			return nullptr;
		}
	}
	Connection* c = _connections.front();
	_connections.pop();
	// TODO: hide postgres here - move into connection
	if (PQstatus(c->connection()) == CONNECTION_OK) {
		return c;
	}

	if (c->connect()) {
		return c;
	}

	delete c;
	--_connectionAmount;
	return connection();
}

}
