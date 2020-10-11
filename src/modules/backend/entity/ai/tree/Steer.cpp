/**
 * @file
 */

#include "Steer.h"
#include "backend/entity/ai/common/Math.h"
#include "backend/entity/ai/common/Random.h"
#include "backend/entity/ai/AI.h"
#include "core/StringUtil.h"
#include "core/GLM.h"
#include "core/Assert.h"
#include "core/collection/DynamicArray.h"
#include <glm/gtx/compatibility.hpp>
#include <glm/gtc/constants.hpp>

namespace backend {

Steer::Steer(const core::String& name, const core::String& parameters, const ConditionPtr& condition, const movement::WeightedSteering &w) :
		ITask(name, parameters, condition), _w(w) {
	_type = "Steer";
}

TreeNodePtr Steer::Factory::create (const SteerNodeFactoryContext *ctx) const {
	movement::WeightedSteerings weightedSteerings;

	if (ctx->parameters.empty()) {
		for (const SteeringPtr& s : ctx->steerings) {
			weightedSteerings.push_back(movement::WeightedData(s, 1.0f));
		}
	} else {
		core::DynamicArray<core::String> tokens;
		core::string::splitString(ctx->parameters, tokens, ",");
		core_assert_msg(tokens.size() == ctx->steerings.size(), "weights doesn't match steerings methods count");
		const int tokenAmount = static_cast<int>(tokens.size());
		for (int i = 0; i < tokenAmount; ++i) {
			weightedSteerings.push_back(movement::WeightedData(ctx->steerings[i], core::string::toFloat(tokens[i])));
		}
	}
	const movement::WeightedSteering w(weightedSteerings);
	return std::make_shared<Steer>(ctx->name, ctx->parameters, ctx->condition, w);
}

ai::TreeNodeStatus Steer::doAction(const AIPtr& entity, int64_t deltaMillis) {
	const ICharacterPtr& chr = entity->getCharacter();
	const double speed = chr->getCurrent(attrib::Type::SPEED);
	const MoveVector& mv = _w.execute(entity, (float)speed);
	const glm::vec3& direction = mv.getVector();
	if (!mv.isValid()) {
		return ai::TreeNodeStatus::FAILED;
	}
	glm_assert_vec3(direction);

	const float deltaSeconds = static_cast<float>(deltaMillis) / 1000.0f;
	const glm::vec3& pos = chr->getPosition();
	const glm::vec3 newPos = pos + (direction * deltaSeconds);
	chr->setPosition(newPos);
	const float src = chr->getOrientation() - glm::pi<float>();
	const float dest = mv.getRotation() - glm::pi<float>();
	const float lerpedRot = glm::lerp(src, dest, deltaSeconds) + glm::pi<float>();
	chr->setOrientation(lerpedRot);
	return ai::TreeNodeStatus::FINISHED;
}

}
