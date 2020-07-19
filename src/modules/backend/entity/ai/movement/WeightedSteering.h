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

	WeightedData(const SteeringPtr& _steering, float _weight = 1.0f);
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
	explicit WeightedSteering(const WeightedSteerings& steerings);

	MoveVector execute (const AIPtr& ai, float speed) const;
};

}
}
