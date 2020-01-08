/**
 * @file
 */

#include "DBCondition.h"
#include "Model.h"
#include "core/String.h"
#include "core/Common.h"
#include "core/Assert.h"
#include "core/ArrayLength.h"
#include <SDL_stdinc.h>

namespace persistence {

namespace __priv {

static const char *comparatorString[] = {
	"=",
	"!=",
	">",
	"<",
	">=",
	"<=",
	"LIKE",
	"IN",
	"NOT IN"
};
static_assert(std::enum_value(Comparator::Max) == lengthof(comparatorString), "Comparator array sizes don't match");

}

DBCondition::DBCondition(const char* field, FieldType type, const std::string& value, Comparator comp) :
		_comp(comp), _field(field), _valueCopy(SDL_strdup(value.c_str())), _value(_valueCopy), _type(type) {
}

DBCondition::~DBCondition() {
	if (_valueCopy != nullptr) {
		SDL_free(_valueCopy);
		_valueCopy = nullptr;
	}
}

std::string DBCondition::statement(int& parameterCount) const {
	if (_field == nullptr) {
		return "";
	}
	if (_comp == Comparator::Max) {
		return "";
	}
	++parameterCount;
	const char* c = __priv::comparatorString[std::enum_value(_comp)];
	if (_type == FieldType::PASSWORD) {
		return core::string::format("\"%s\" %s crypt($%i, \"%s\")", _field, c, parameterCount, _field);
	}
	return core::string::format("\"%s\" %s $%i", _field, c, parameterCount);
}

std::string DBConditionOne::statement(int& parameterCount) const {
	static const std::string empty = "";
	return empty;
}

std::string DBConditionMultiple::statement(int& parameterCount) const {
	const char* o = _and ? " AND " : " OR ";
	return core::string::join(_conditions.begin(), _conditions.end(), o, [&] (const DBCondition* cond) {
		const std::string& s = cond->statement(parameterCount);
		Log::debug("Statement: '%s', parameterCount: %i", s.c_str(), parameterCount);
		return s;
	});
}

const char* DBConditionMultiple::value(int index) const {
	// TODO: the second index is nonsense
	return _conditions[index]->value(index);
}


}
