/**
 * @file
 */

#include "ScopedConnection.h"
#include "Connection.h"

namespace persistence {

ScopedConnection::~ScopedConnection() {
	_c->close();
}

}
