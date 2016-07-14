/**
 * @file
 */

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

	virtual ~PeristenceModel();

	const std::string& getTableName() const;

	virtual std::string getCreate() const = 0;

	virtual Fields getFields() const = 0;

	virtual bool isSerial(const std::string& fieldname) const = 0;
};

inline const std::string& PeristenceModel::getTableName() const {
	return _tableName;
}

}
