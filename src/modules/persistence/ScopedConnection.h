#pragma once

#include <libpq-fe.h>

namespace persistence {

class Connection;

class ScopedConnection {
private:
	Connection* _c;
public:
	ScopedConnection(Connection* c) :
			_c(c) {
	}

	inline operator Connection*() {
		return _c;
	}

	operator PGconn*();

	inline operator bool() const {
		return _c != nullptr;
	}

	~ScopedConnection();
};

}
