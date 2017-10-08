/**
 * @file
 */

#pragma once

#include "FieldType.h"
#include <string>
#include <vector>

namespace persistence {

class Model;
struct Field;

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

}
