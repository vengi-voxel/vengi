#include "Persister.h"
#include "UserStore.h"
#include "core/Log.h"
#include "core/Var.h"

namespace backend {

Persister::Persister() :
		_connection(), _store(&_connection) {
}

Persister::~Persister() {
	_connection.disconnect();
}

bool Persister::storeUser(const std::string& mail, const std::string& passwd, const std::string& uid) {
	UserStore dbUser(mail, passwd, uid);
	return _store.storeModel(dbUser);
}

int Persister::loadUser(const std::string& mail, const std::string& passwd, const std::string& uid) {
	const UserStore dbUser(mail, passwd, uid);
	_userData = std::move(_store.loadModel(dbUser));
	if (!_userData.empty()) {
		const int uid = core::string::toInt(_userData["userid"]);
		return uid;
	}
	return 0;
}

void Persister::close() {
	_connection.disconnect();
}

bool Persister::initTables() {
	return true;
}

bool Persister::init() {
	Log::trace("init database connection");
	const core::VarPtr& dbName = core::Var::get("db_name", "engine_db");
	const core::VarPtr& dbHost = core::Var::get("db_host", "localhost");
	const core::VarPtr& dbPw = core::Var::get("db_pw", "ben711cCefIUit887");
	const core::VarPtr& dbUser = core::Var::get("db_user", "dbmaster");

	_connection.changeDb(dbName->strVal());
	_connection.changeHost(dbHost->strVal());
	_connection.setLoginData(dbUser->strVal(), dbPw->strVal());

	return _connection.connect();
}

}
