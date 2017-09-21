/**
 * @file
 */

#pragma once

#include "Model.h"
#include "core/String.h"

namespace persistence {

class DBHandler {
private:
	static std::string quote(const std::string& in) {
		return core::string::format("\"%s\"", in.c_str());
	}
public:
	template<class MODEL>
	static void truncate(const MODEL& model) {
		model.exec("TRUNCATE TABLE " + quote(model.getTableName()));
	}

	template<class MODEL>
	static void truncate(MODEL&& model) {
		model.exec("TRUNCATE TABLE " + quote(model.getTableName()));
	}
};

}
