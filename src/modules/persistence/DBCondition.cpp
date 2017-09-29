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

std::string DBCondition::statement(int& parameterCount) const {
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

std::string DBConditionOne::statement(int& parameterCount) const {
	static const std::string one = "true";
	return one;
}

std::string DBConditionMultiple::statement(int& parameterCount) const {
	const char* o = _and ? " AND " : " OR ";
	return core::string::join(_conditions.begin(), _conditions.end(), o, [&] (const DBCondition& cond) {
		return cond.statement(parameterCount);
	});
}

}
