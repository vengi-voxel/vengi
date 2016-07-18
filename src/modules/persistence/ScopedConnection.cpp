#include "ScopedConnection.h"

namespace persistence {

ScopedConnection::~ScopedConnection() {
	_c->close();
}

}
