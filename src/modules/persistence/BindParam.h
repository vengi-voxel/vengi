/**
 * @file
 */

#pragma once

#include "FieldType.h"
#include "core/String.h"
#include <vector>

namespace persistence {

class Model;
struct Field;

struct BindParam {
	std::vector<const char *> values;
	std::vector<int> lengths;
	std::vector<int> formats;
	std::vector<core::String> valueBuffers;
	std::vector<FieldType> fieldTypes;
	/**
	 * @brief The real amount of added fields
	 */
	int position = 0;
	/**
	 * @brief The amount of expected fields.
	 * @see @c add()
	 */
	BindParam(int num);

	/**
	 * @return The index of the position in the buffer to add a value to.
	 * @note Might trigger a reallocate if the initial buffer was not big enough.
	 */
	int add();
	/**
	 * @brief Pushes a new value for the given field of the given model to the parameter
	 */
	void push(const Model& model, const Field& field);
};

}
