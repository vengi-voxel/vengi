#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <unordered_map>

namespace persistence {

typedef std::unordered_map<std::string, std::string> Fields;

class PeristenceModel {
protected:
	const std::string _tableName;
public:
	PeristenceModel(const std::string& tableName);

	virtual ~PeristenceModel() {
	}

	virtual std::string getCreate() const = 0;

	virtual const std::string& getTableName() const {
		return _tableName;
	}

	virtual Fields getFields() const = 0;

	virtual void update(const std::string& fieldName,const std::string& value) const = 0;

	virtual bool isSerial(const std::string& fieldname) const = 0;
};

}
