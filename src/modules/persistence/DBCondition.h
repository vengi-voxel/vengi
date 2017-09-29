/**
 * @file
 */

#pragma once

#include <string>

namespace persistence {

class Model;

enum class Operator {
	Equal,
	NotEqual,
	Bigger,
	Lesser,
	BiggerOrEqual,
	LessOrEqual,

	Max
};

class DBCondition {
protected:
	Operator _op = Operator::Max;
	const char* _field = nullptr;
	const char* _value = nullptr;

	constexpr DBCondition() {}
public:
	constexpr DBCondition(const char* field, const char* value, Operator op = Operator::Equal) :
			_op(op), _field(field), _value(value) {
	}

	virtual ~DBCondition() {
	}

	/**
	 * @return A string - and never @c nullptr (but can be empty of course) - that the field of the
	 * query is checked against.
	 */
	const char* value() const;

	virtual std::string statement(const Model& model, int& parameterCount) const;
};

inline const char* DBCondition::value() const {
	if (_value == nullptr) {
		return "";
	}
	return _value;
}

class DBConditionOne : public DBCondition {
private:
	using Super = DBCondition;
public:
	constexpr DBConditionOne() :
			Super() {
	}

	std::string statement(const Model& model, int& parameterCount) const override;
};

}
