#pragma once

#include "Connection.h"

namespace persistence {

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

	inline Connection* connection() {
		return _c;
	}

	inline operator bool() const {
		return _c != nullptr;
	}

	~ScopedConnection();
};

}
