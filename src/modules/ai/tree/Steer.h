/**
 * @file
 */
#pragma once

#include "tree/ITask.h"
#include "common/StringUtil.h"
#include "common/Common.h"
#include "common/Math.h"
#include "movement/Steering.h"
#include "movement/WeightedSteering.h"
#include "common/Random.h"
#include "common/Assert.h"

namespace ai {

class Steer: public ITask {
protected:
	const movement::WeightedSteering _w;
public:
	Steer(const core::String& name, const core::String& parameters, const ConditionPtr& condition, const movement::WeightedSteering &w) :
			ITask(name, parameters, condition), _w(w) {
		_type = "Steer";
	}
	class Factory: public ISteerNodeFactory {
	public:
		TreeNodePtr create (const SteerNodeFactoryContext *ctx) const override {
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
			return std::make_shared<Steer>(ctx->name, ctx->parameters, ctx->condition, w);
		}
	};
	static const Factory& getFactory() {
		static Factory FACTORY;
		return FACTORY;
	}

	TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) override {
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
};

}
