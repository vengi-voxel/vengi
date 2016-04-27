#include "Steer.h"
#include "common/Random.h"
#include "movement/Steering.h"
#include <iostream>

namespace ai {

TreeNodeStatus Steer::doAction(const AIPtr& entity, int64_t deltaMillis) {
	const ICharacterPtr& chr = entity->getCharacter();
	const MoveVector& mv = _w.execute(entity, chr->getSpeed());
	if (isInfinite(mv.getVector())) {
		return FAILED;
	}

	const float deltaSeconds = static_cast<float>(deltaMillis) / 1000.0f;
	chr->setPosition(chr->getPosition() + (mv.getVector() * deltaSeconds));
	chr->setOrientation(fmodf(chr->getOrientation() + mv.getRotation() * deltaSeconds, glm::two_pi<float>()));
	return FINISHED;
}

TreeNodePtr Steer::Factory::create(const SteerNodeFactoryContext *ctx) const {
	movement::WeightedSteerings weightedSteerings;

	if (ctx->parameters.empty()) {
		for (const SteeringPtr& s : ctx->steerings) {
			weightedSteerings.push_back(movement::WeightedData(s, 1.0f));
		}
	} else {
		std::vector<std::string> tokens;
		Str::splitString(ctx->parameters, tokens, ",");
		ai_assert(tokens.size() == ctx->steerings.size(), "weights doesn't match steerings methods count");
		const int tokenAmount = static_cast<int>(tokens.size());
		for (int i = 0; i < tokenAmount; ++i) {
			weightedSteerings.push_back(movement::WeightedData(ctx->steerings[i], Str::strToFloat(tokens[i])));
		}
	}
	const movement::WeightedSteering w(weightedSteerings);
	return TreeNodePtr(new Steer(ctx->name, ctx->parameters, ctx->condition, w));
}

std::ostream& Steer::print(std::ostream& stream, int level) const {
	ITask::print(stream, level);
	stream << " => {" << std::endl;
	_w.print(stream, level);
	for (int i = 0; i < level; ++i) {
		stream << '\t';
	}
	stream << "}";
	return stream;
}

Steer::Factory Steer::FACTORY;

}
