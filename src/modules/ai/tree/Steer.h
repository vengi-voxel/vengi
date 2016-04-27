#pragma once

#include "tree/ITask.h"
#include "common/String.h"
#include "common/Types.h"
#include "common/Math.h"
#include "movement/Steering.h"
#include "movement/WeightedSteering.h"

namespace ai {

class Steer: public ITask {
protected:
	const movement::WeightedSteering _w;
public:
	Steer(const std::string& name, const std::string& parameters, const ConditionPtr& condition, const movement::WeightedSteering &w) :
			ITask(name, parameters, condition), _w(w) {
		_type = "Steer";
	}
	class Factory: public ISteerNodeFactory {
	public:
		TreeNodePtr create (const SteerNodeFactoryContext *ctx) const override;
	};
	static Factory FACTORY;

	TreeNodeStatus doAction(const AIPtr& entity, int64_t deltaMillis) override;

	std::ostream& print(std::ostream& stream, int level) const override;
};

}
