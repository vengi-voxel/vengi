#include "Not.h"

namespace ai {

ConditionPtr Not::Factory::create(const ConditionFactoryContext *ctx) const {
	if (ctx->conditions.size() != 1)
		return ConditionPtr();
	return ConditionPtr(new Not(ctx->conditions.front()));
}

std::ostream& Not::print(std::ostream& stream, int level) const {
	ICondition::print(stream, level);
	stream << "(";
	_condition->print(stream, level);
	stream << ")";
	return stream;
}

Not::Factory Not::FACTORY;

}
