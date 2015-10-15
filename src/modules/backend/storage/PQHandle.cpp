#include "PQHandle.h"
#include "dbpost/StoreInterface.h"
#include "UserStore.h"
#include "core/Log.h"
#include "core/Var.h"

namespace backend {

PQHandle::PQHandle() :
		_pqConnection(), _pqStore(&_pqConnection) {
}

PQHandle::~PQHandle() {
	_pqConnection.disconnect();
}

void PQHandle::storeUser(const std::string& mail, const std::string& passwd, const std::string& uid) {
	UserStore dbUser(mail, passwd, uid);
	_pqStore.storeModel(dbUser);
}

int PQHandle::loadUser(const std::string& mail, const std::string& passwd, const std::string& uid) {
	const UserStore dbUser(mail, passwd, uid);
	_userData = std::move(_pqStore.loadModel(dbUser));
	if (_userData.size() > 0) {
		const int uid = std::stoi(_userData["userid"]);
		return uid;
	}
	return 0;
}
void PQHandle::close() {
	_pqConnection.disconnect();
}

void PQHandle::initTables() {
	std::string tmUid = "0";
	std::string tmUser = std::string("a");
	std::string tmPw = std::string("b");
	UserStore dbUser(tmUser, tmPw, tmUid);
	_pqStore.createNeeds(dbUser);
}

void PQHandle::init() {
	Log::trace("init database connection");
	const core::VarPtr& dbName = core::Var::get("db_name", "engine_db");
	const core::VarPtr& dbHost = core::Var::get("db_host", "localhost");
	const core::VarPtr& dbPw = core::Var::get("db_pw", "ben711cCefIUit887");
	const core::VarPtr& dbUser = core::Var::get("db_user", "dbmaster");

	_pqConnection.changeDb(dbName->strVal());
	_pqConnection.changeHost(dbHost->strVal());
	_pqConnection.setLoginData(dbUser->strVal(), dbPw->strVal());

	if (_pqConnection.connect() == 0) {
		Log::debug("database connection established");
	} else {
		Log::error("database connection failed");
	}
}

}
