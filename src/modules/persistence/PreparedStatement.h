/**
 * @file
 */

#pragma once

#include "FieldType.h"
#include "Timestamp.h"
#include "State.h"
#include <string>
#include <vector>

namespace persistence {

class Model;
class Field;

struct BindParam {
	std::vector<const char *> values;
	std::vector<int> lengths;
	std::vector<int> formats;
	std::vector<std::string> valueBuffers;
	std::vector<FieldType> fieldTypes;
	int position = 0;
	BindParam(int num);

	int add();
	void push(const Model& model, const Field& field);
};

class PreparedStatement {
private:
	Model* _model;
	std::string _name;
	std::string _statement;
	BindParam _params;
public:
	PreparedStatement(Model* model, const std::string& name, const std::string& statement);

	PreparedStatement& add(const std::string& value, FieldType fieldType);

	PreparedStatement& add(const std::string& value);

	PreparedStatement& add(int value);

	PreparedStatement& add(long value);

	PreparedStatement& addPassword(const std::string& password);

	PreparedStatement& addPassword(const char* password);

	PreparedStatement& add(const char* value, FieldType fieldType);

	PreparedStatement& add(nullptr_t value, FieldType fieldType);

	PreparedStatement& add(const Timestamp& type);
	State exec();
};

}
