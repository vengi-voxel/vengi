#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "persistence/Store.h"
#include "persistence/Connection.h"

namespace backend {

class Persister {
private:
	persistence::Connection _connection;
	persistence::Store _store;
	persistence::KeyValueMap _userData;
public:
	Persister();
	virtual ~Persister();

	bool init();

	void close();

	bool initTables();

	bool storeUser(const std::string& uMail, const std::string& uPasswd, const std::string& uid);

	int loadUser(const std::string& uMail, const std::string& uPasswd, const std::string& uid);
};

}
