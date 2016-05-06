/**
 * @file
 */

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include "persistence/PeristenceModel.h"

#pragma once

namespace backend {

class UserStore: public persistence::PeristenceModel {
private:
	std::string _email;
	std::string _password;
	std::string _userid;
public:
	UserStore(const std::string& email, const std::string& password, const std::string& userid);

	std::string getCreate() const override;

	persistence::Fields getFields() const override;

	bool isSerial(const std::string& fieldname) const override;

	void update(const std::string& fieldName, const std::string& value) const override;
};

}
