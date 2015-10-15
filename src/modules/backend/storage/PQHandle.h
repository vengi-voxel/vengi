#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "dbpost/PQStore.h"
#include "dbpost/PQConnect.h"

namespace backend {

class PQHandle {
private:
	dbpost::PQConnect _pqConnection;
	dbpost::PQStore _pqStore;
	std::unordered_map<std::string, std::string> _userData;
public:
	PQHandle();
	void init();
	void close();
	void initTables();
	void storeUser(const std::string& uMail, const std::string& uPasswd, const std::string& uid);
	int loadUser(const std::string& uMail, const std::string& uPasswd, const std::string& uid);
	virtual ~PQHandle();
};

}
