/**
 * @file
 */
#pragma once

#include "ITask.h"
#include "backend/entity/ai/movement/Steering.h"
#include "backend/entity/ai/movement/WeightedSteering.h"

namespace backend {

class Steer: public ITask {
protected:
	const movement::WeightedSteering _w;
public:
	Steer(const core::String& name, const core::String& parameters, const ConditionPtr& condition, const movement::WeightedSteering &w);

	class Factory: public ISteerNodeFactory {
	public:
		TreeNodePtr create (const SteerNodeFactoryContext *ctx) const override;
	};
	static const Factory& getFactory() {
		static Factory FACTORY;
		return FACTORY;
	}

	ai::TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) override;
};

}
