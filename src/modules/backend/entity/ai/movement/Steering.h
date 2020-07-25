/**
 * @file
 * @brief Defines some basic movement algorithms like Wandering, Seeking and Fleeing.
 */
#pragma once

#include "backend/entity/ai/common/MemoryAllocator.h"
#include "backend/entity/ai/common/MoveVector.h"
#include "backend/entity/ai/AIFactories.h"

namespace backend {

class AI;
typedef std::shared_ptr<AI> AIPtr;

namespace movement {

#define STEERING_FACTORY(SteeringName) \
public: \
	class Factory: public ISteeringFactory { \
	public: \
		SteeringPtr create (const SteeringFactoryContext *ctx) const override { \
			return std::make_shared<SteeringName>(ctx->parameters); \
		} \
	}; \
	static const Factory& getFactory() { \
		static Factory FACTORY; \
		return FACTORY; \
	}

#define STEERING_FACTORY_SINGLETON \
public: \
	class Factory: public ISteeringFactory { \
		SteeringPtr create (const SteeringFactoryContext */*ctx*/) const { \
			return get(); \
		} \
	}; \
	static const Factory& getFactory() { \
		static Factory FACTORY; \
		return FACTORY; \
	}

/**
 * @brief Steering interface
 */
class ISteering : public MemObject {
public:
	virtual ~ISteering() {}
	/**
	 * @brief Calculates the @c MoveVector
	 *
	 * @return If the @c MoveVector contains @c glm::vec3::VEC3_INFINITE as vector, the result should not be used
	 * because there was an error.
	 */
	virtual MoveVector execute (const AIPtr& ai, float speed) const = 0;
};

/**
 * @brief @c IFilter steering interface
 */
class SelectionSteering : public ISteering {
protected:
	bool getSelectionTarget(const AIPtr& entity, size_t index, glm::vec3& position) const;

public:
	virtual ~SelectionSteering() {}
};

}
}
