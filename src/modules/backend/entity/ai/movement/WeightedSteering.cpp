/**
 * @file
 */

#include "WeightedSteering.h"
#include "backend/entity/ai/AI.h"
#include "backend/entity/ai/common/Math.h"

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
	for (const WeightedData& wd : _steerings) {
		const float weight = wd.weight;
		const MoveVector& mv = wd.steering->execute(ai, speed);
		if (isInfinite(mv.getVector())) {
			continue;
		}

		vecBlended += mv.getVector() * weight;
		angularBlended += mv.getRotation() * weight;
		totalWeight += weight;
	}

	if (totalWeight <= 0.0000001f) {
		return MoveVector(VEC3_INFINITE, 0.0f);
	}

	const float scale = 1.0f / totalWeight;
	return MoveVector(vecBlended * scale, fmodf(angularBlended * scale, glm::two_pi<float>()));
}

}
}
