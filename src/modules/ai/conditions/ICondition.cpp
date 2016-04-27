#include "conditions/ICondition.h"

namespace ai {

int ICondition::_nextId = 0;

std::ostream& ICondition::print(std::ostream& stream, int) const {
	stream << _name;
	if (!_parameters.empty()) {
		stream << "(\"";
		stream << _parameters;
		stream << "\")";
	}
	return stream;
}

}
