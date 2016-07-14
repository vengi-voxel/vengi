/**
 * @file
 */

#include "UserStore.h"
#include "core/Log.h"
#include "backend/entity/User.h"

namespace backend {

UserStore::UserStore(const std::string& email, const std::string& password, const std::string& userid) :
		persistence::PeristenceModel("user_table"), _email(email), _password(password), _userid(userid) {
}

std::string UserStore::getCreate() const {
	const std::string crSql = "CREATE TABLE " + getTableName() + " ( userid bigserial primary key,"
			" user_email varchar(180) UNIQUE,"
			" user_pw_hash varchar(60) "
			");";
	return crSql;
}

persistence::Fields UserStore::getFields() const {
	persistence::Fields storeData;
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
