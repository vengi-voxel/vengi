#include "ScopedConnection.h"
#include "Connection.h"

namespace persistence {

ScopedConnection::operator PGconn*() {
	return _c->connection();
}

ScopedConnection::~ScopedConnection() {
	_c->close();
}

}
