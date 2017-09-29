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

DBCondition::DBCondition(Operator op) :
		_op(op) {
}

DBCondition::~DBCondition() {
}

std::string DBCondition::value(int index, const Model& model) const {
	return "42";
}

std::string DBCondition::field(int index, const Model& model) const {
	return "FIELD";
}

std::string DBCondition::statement(const Model& model, int& parameterCount) const {
	++parameterCount;
	const std::string& f = field(parameterCount, model);
	const char* o = __priv::operatorString[std::enum_value(_op)];
	return core::string::format("%s %s $%i", f.c_str(), o, parameterCount);
}

DBConditionOne::DBConditionOne() :
		Super(Operator::Max) {
}

std::string DBConditionOne::statement(const Model& model, int& parameterCount) const {
	return "1";
}

}
