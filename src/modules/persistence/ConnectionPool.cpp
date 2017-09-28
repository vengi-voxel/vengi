/**
 * @file
 */

#include "ConnectionPool.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Common.h"
#include "core/GameConfig.h"

namespace persistence {

ConnectionPool::ConnectionPool() {
}

ConnectionPool::~ConnectionPool() {
	shutdown();
}

int ConnectionPool::init() {
	_min = core::Var::getSafe(cfg::DatabaseMinConnections)->intVal();
	_max = core::Var::getSafe(cfg::DatabaseMaxConnections)->intVal();

	core_assert_always(_min <= _max);

	_dbName = core::Var::getSafe(cfg::DatabaseName);
	_dbHost = core::Var::getSafe(cfg::DatabaseHost);
	_dbUser = core::Var::getSafe(cfg::DatabaseUser);
	_dbPw = core::Var::getSafe(cfg::DatabasePassword);

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
	if (c == nullptr) {
		return;
	}
	_connections.push(c);
}

Connection* ConnectionPool::connection() {
	if (_connections.empty()) {
		if (_connectionAmount >= _max) {
			Log::warn("Could not acquire pooled connection, max limit hit");
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
	if (c->status()) {
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
