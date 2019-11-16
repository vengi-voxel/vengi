/**
 * @file
 */

#include "ScopedConnection.h"
#include "Connection.h"
#include "ConnectionPool.h"

namespace persistence {

ScopedConnection::~ScopedConnection() {
	_connectionPool.giveBack(_c);
}

}
