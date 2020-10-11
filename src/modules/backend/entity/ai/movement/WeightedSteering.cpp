/**
 * @file
 */

#include "WeightedSteering.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"
#include "common/MoveVector.h"
#include "core/Assert.h"
#include "core/Log.h"
#include <glm/common.hpp>
#include <glm/gtc/constants.hpp>

namespace backend {
namespace movement {

WeightedData::WeightedData(const SteeringPtr& _steering, float _weight) :
		steering(_steering), weight(_weight) {
	core_assert_msg(weight > 0.0001f, "Weight is too small");
}

WeightedSteering::WeightedSteering(const WeightedSteerings& steerings) :
		_steerings(steerings) {
}

MoveVector WeightedSteering::execute (const AIPtr& ai, float speed) const {
	float totalWeight = 0.0f;
	glm::vec3 vecBlended(0.0f);
	float angularBlended = 0.0f;
	MoveVectorState state = MoveVectorState::Invalid;
	for (const WeightedData& wd : _steerings) {
		const float weight = wd.weight;
		const MoveVector& mv = wd.steering->execute(ai, speed);
		state = mv.state();
		if (!mv.isValid()) {
			continue;
		}

		vecBlended += mv.getVector() * weight;
		angularBlended += mv.getRotation() * weight;
		totalWeight += weight;
	}

	if (state == MoveVectorState::Invalid) {
		return MoveVector::Invalid;
	}
	if (state == MoveVectorState::TargetReached) {
		return MoveVector::TargetReached;
	}

	const float scale = 1.0f / totalWeight;
	return MoveVector(vecBlended * scale, glm::mod(angularBlended * scale, glm::two_pi<float>()));
}

}
}
