/**
 * @file
 * @brief Defines some basic movement algorithms like Wandering, Seeking and Fleeing.
 */
#pragma once

#include "AI.h"
#include "zone/Zone.h"
#include "IAIFactory.h"
#include "ai-shared/common/Math.h"
#include "ai-shared/common/MoveVector.h"
#include "ai-shared/common/StringUtil.h"
#include "ai-shared/common/MemoryAllocator.h"
#include "ICharacter.h"

namespace ai {
namespace movement {

#define STEERING_FACTORY(SteeringName) \
public: \
	class Factory: public ::ai::ISteeringFactory { \
	public: \
		::ai::SteeringPtr create (const ::ai::SteeringFactoryContext *ctx) const override { \
			return std::make_shared<SteeringName>(ctx->parameters); \
		} \
	}; \
	static const Factory& getFactory() { \
		static Factory FACTORY; \
		return FACTORY; \
	}

#define STEERING_FACTORY_SINGLETON \
public: \
	class Factory: public ::ai::ISteeringFactory { \
		::ai::SteeringPtr create (const ::ai::SteeringFactoryContext */*ctx*/) const { \
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
	glm::vec3 getSelectionTarget(const AIPtr& entity, std::size_t index) const {
		const FilteredEntities& selection = entity->getFilteredEntities();
		if (selection.empty() || selection.size() <= index) {
			return VEC3_INFINITE;
		}
		const Zone* zone = entity->getZone();
		const CharacterId characterId = selection[index];
		const AIPtr& ai = zone->getAI(characterId);
		const ICharacterPtr character = ai->getCharacter();
		return character->getPosition();
	}

public:
	virtual ~SelectionSteering() {}
};

}
}
