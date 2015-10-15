#include "UserStore.h"
#include "core/Log.h"
#include "backend/entity/User.h"

namespace backend {

UserStore::UserStore(const std::string& email, const std::string& password, const std::string& userid) :
		_email(email), _password(password), _userid(userid) {
}

std::string UserStore::getCreate() const {
	const std::string crSql = "CREATE TABLE " + getTableName() + " ( userid bigserial primary key,"
			" user_email varchar(180) UNIQUE,"
			" user_pw_hash varchar(60) "
			");";
	return crSql;
}

void UserStore::update(const std::string& fieldName, const std::string& value) const {
	Log::trace("update called");
	if (fieldName == "userid") {
		Log::trace("userid read %s", value.c_str());
	}
}

std::string UserStore::getTableName() const {
	static const std::string tabName = "user_table";
	return tabName;
}

std::unordered_map<std::string, std::string> UserStore::getFields() const {
	std::unordered_map<std::string, std::string> storeData;
	storeData["userid"] = _userid;
	storeData["user_email"] = _email;
	storeData["user_pw_hash"] = _password;

	return storeData;
}

bool UserStore::isSerial(const std::string& fieldname) const {
	if (fieldname == "userid") {
		return true;
	}
	return false;
}

}
