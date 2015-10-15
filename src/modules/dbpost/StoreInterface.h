#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <unordered_map>

namespace dbpost {

class StoreInterface {
public:
	virtual ~StoreInterface() {
	}
	virtual std::string getCreate() const = 0;
	virtual std::string getTableName() const = 0;
	virtual std::unordered_map<std::string, std::string> getFields() const = 0;
	virtual void update(const std::string& fieldName,const std::string& value) const = 0;
	virtual bool isSerial(const std::string& fieldname) const = 0;
};

}
