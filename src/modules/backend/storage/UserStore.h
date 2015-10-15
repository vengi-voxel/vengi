#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "dbpost/StoreInterface.h"

#pragma once

namespace backend {

class UserStore: public dbpost::StoreInterface {
private:
	std::string _email;
	std::string _password;
	std::string _userid;
	std::unordered_map<std::string, std::string> _storage;
public:
	UserStore(const std::string& email, const std::string& password, const std::string& userid);

	std::string getCreate() const override;

	std::unordered_map<std::string, std::string> getFields() const override;

	bool isSerial(const std::string& fieldname) const override;

	virtual void update(const std::string& fieldName, const std::string& value) const override;

	std::string getTableName() const override;
};

}
