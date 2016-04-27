#include "Print.h"
#include "ICharacter.h"

namespace ai {

void Print::tree(const AIPtr& entity) const {
	const TreeNodePtr behaviour = entity->getBehaviour();
	std::cout << behaviour.get() << std::endl;
}

void Print::attributes(const AIPtr& entity) const {
	const CharacterAttributes& attribs = entity->getCharacter()->getAttributes();
	for (auto i = attribs.begin(); i != attribs.end(); ++i) {
		std::cout << i->first << ": " << i->second << std::endl;
	}
}

TreeNodeStatus Print::handleCommand(const AIPtr& entity) const {
	if (_parameters == "::tree") {
		tree(entity);
	} else if (_parameters == "::attributes") {
		attributes(entity);
	} else {
		std::cout << "Unknown command: " << _parameters << std::endl;
		return FAILED;
	}
	return FINISHED;
}

TreeNodeStatus Print::doAction(const AIPtr& entity, int64_t /*deltaMillis*/) {
	if (ai::Str::startsWith(_parameters, "::")) {
		return handleCommand(entity);
	}
	std::cout << _parameters << std::endl;
	return FINISHED;
}

NODE_FACTORY_IMPL(Print)

}
