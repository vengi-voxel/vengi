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
private:
	Operator _op;
public:
	DBCondition(Operator op);
	virtual ~DBCondition();

	virtual std::string value(int index, const Model& model) const;

	virtual std::string field(int index, const Model& model) const;

	virtual std::string statement(const Model& model, int& parameterCount) const;
};

class DBConditionOne : public DBCondition {
private:
	using Super = DBCondition;
public:
	DBConditionOne();

	std::string statement(const Model& model, int& parameterCount) const override;
};

}
