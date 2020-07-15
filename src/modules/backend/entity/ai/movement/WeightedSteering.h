/**
 * @file
 */
#pragma once

#include "Steering.h"
#include "core/Assert.h"

namespace backend {
namespace movement {

/**
 * @brief Steering and weight as input for @c WeightedSteering
 */
struct WeightedData {
	SteeringPtr steering;
	const float weight;

	WeightedData(const SteeringPtr& _steering, float _weight = 1.0f) :
			steering(_steering), weight(_weight) {
		core_assert_msg(weight > 0.0001f, "Weight is too small");
	}
};
typedef std::vector<WeightedData> WeightedSteerings;
typedef WeightedSteerings::const_iterator WeightedSteeringsIter;

/**
 * @brief This class allows you to weight several steering methods and get a blended @c MoveVector out of it.
 */
class WeightedSteering {
private:
	WeightedSteerings _steerings;
public:
	explicit WeightedSteering(const WeightedSteerings& steerings) :
			_steerings(steerings) {
	}

	MoveVector execute (const AIPtr& ai, float speed) const {
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
};

}
}
