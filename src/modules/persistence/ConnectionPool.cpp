/**
 * @file
 */

#include "ConnectionPool.h"
#include "core/Log.h"
#include "core/Var.h"
#include "core/Assert.h"
#include "core/GameConfig.h"

namespace persistence {

ConnectionPool::ConnectionPool() {
}

ConnectionPool::~ConnectionPool() {
	shutdown();
}

bool ConnectionPool::init() {
	std::unique_lock lock(_mutex);
	_minConnections = core::Var::getSafe(cfg::DatabaseMinConnections);
	_maxConnections = core::Var::getSafe(cfg::DatabaseMaxConnections);

	_min = _minConnections->intVal();
	_max = _maxConnections->intVal();

	if (_min > _max) {
		Log::error("The min connection amount must be smaller or equal to the max connection amount");
		return false;
	}

	_dbName = core::Var::getSafe(cfg::DatabaseName);
	_dbHost = core::Var::getSafe(cfg::DatabaseHost);
	_dbUser = core::Var::getSafe(cfg::DatabaseUser);
	_dbPw = core::Var::getSafe(cfg::DatabasePassword);

	Log::debug("Connect to %s@%s to database %s", _dbUser->strVal().c_str(), _dbHost->strVal().c_str(), _dbName->strVal().c_str());

	for (int i = _connectionAmount; i < _min; ++i) {
		addConnection();
	}

	if (_connectionAmount > 0) {
		return true;
	}
	Log::error("Failed to connect to %s@%s to database %s", _dbUser->strVal().c_str(), _dbHost->strVal().c_str(), _dbName->strVal().c_str());
	return false;
}

void ConnectionPool::shutdown() {
	std::unique_lock lock(_mutex);
	while (!_connections.empty()) {
		Connection* c = _connections.front();
		c->disconnect();
		_connections.pop();
		--_connectionAmount;
		delete c;
	}
	if (_connectionAmount != 0) {
		Log::warn("Connections out of sync: %i", _connectionAmount);
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
	if (!c->connect()) {
		delete c;
		return nullptr;
	}

	_connections.push(c);
	++_connectionAmount;
	return c;
}

void ConnectionPool::giveBack(Connection* c) {
	if (c == nullptr) {
		return;
	}
	std::unique_lock lock(_mutex);
	_connections.push(c);
}

Connection* ConnectionPool::connection() {
	std::unique_lock lock(_mutex);
	if (_connections.empty()) {
		if (_minConnections->isDirty()) {
			const int newMin = _minConnections->intVal();
			if (newMin > 0 && newMin <= _max) {
				_min = newMin;
			}
			_minConnections->markClean();
		}
		if (_maxConnections->isDirty()) {
			const int newMax = _maxConnections->intVal();
			if (newMax > 0 && newMax >= _min) {
				_max = newMax;
			}
			_maxConnections->markClean();
		}
		if (_connectionAmount >= _max) {
			Log::warn("Could not acquire pooled connection, max limit (%i) hit", _max);
			return nullptr;
		}
		Connection* newC = addConnection();
		if (newC == nullptr) {
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
