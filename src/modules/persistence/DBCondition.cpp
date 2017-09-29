/**
 * @file
 */

#include "DBCondition.h"
#include "Model.h"
#include "core/String.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/Array.h"

namespace persistence {

namespace __priv {

static const char *operatorString[] = {
	"=",
	"!=",
	">",
	"<",
	">=",
	"<="
};
static_assert(std::enum_value(Operator::Max) == lengthof(operatorString), "Array sizes don't match");

}

std::string DBCondition::statement(const Model& model, int& parameterCount) const {
	if (_field == nullptr) {
		return "";
	}
	if (_op == Operator::Max) {
		return "";
	}
	++parameterCount;
	const char* o = __priv::operatorString[std::enum_value(_op)];
	return core::string::format("\"%s\" %s $%i", _field, o, parameterCount);
}

std::string DBConditionOne::statement(const Model& model, int& parameterCount) const {
	static const std::string one = "true";
	return one;
}

}
