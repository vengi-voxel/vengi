/**
 * @file
 */

#pragma once

#include "backend/entity/ai/AICommon.h"
#include <atomic>
#include "Entity.h"
#include "ai/AICharacter.h"
#include "backend/poi/PoiProvider.h"

namespace voxel {
class World;
typedef std::shared_ptr<World> WorldPtr;
}

namespace backend {

class EntityStorage;
typedef std::shared_ptr<EntityStorage> EntityStoragePtr;

class Npc: public Entity {
private:
	friend class AICharacter;
	static std::atomic<EntityId> _nextNpcId;
	voxel::WorldPtr _world;
	EntityStoragePtr _entityStorage;
	PoiProviderPtr _poiProvider;
	glm::ivec3 _homePosition;
	ai::AIPtr _ai;

	void moveToGround();

public:
	Npc(network::messages::EntityType type, const EntityStoragePtr& entityStorage, const ai::TreeNodePtr& behaviour, const voxel::WorldPtr& world, const network::MessageSenderPtr& messageSender,
			const core::TimeProviderPtr& timeProvider, const attrib::ContainerProviderPtr& containerProvider, const PoiProviderPtr& poiProvider);
	~Npc();

	void init(const glm::ivec3* pos = nullptr);

	voxel::WorldPtr world() const;
	void setHomePosition(const glm::ivec3& pos);
	/**
	 * @brief Sets a points of interest at the current npc position.
	 */
	void setPointOfInterest();
	const glm::ivec3& homePosition() const;
	bool route(const glm::ivec3& target);
	const ai::AIPtr& ai();

	bool die();
	bool attack(ai::CharacterId id);
	/**
	 * @brief Applies damage to the entity
	 * @param attacker The attacking @c Npc. This might be @c nullptr
	 * @param[in] damage The damage that the attacker tries to apply to the victim.
	 * @return The amount of applied damage.
	 * @note The amount of the applied damage might be less that the indended damage
	 * @note The victim gets aggro on the attacker
	 */
	double applyDamage(Npc* attacker, double damage);

	bool update(long dt) override;

	const glm::vec3& pos() const override;
	float orientation() const override;
	std::string name() const;
};

inline void Npc::setHomePosition(const glm::ivec3& pos) {
	_homePosition = pos;
}

inline voxel::WorldPtr Npc::world() const {
	return _world;
}

inline const glm::ivec3& Npc::homePosition() const {
	return _homePosition;
}

inline const ai::AIPtr& Npc::ai() {
	return _ai;
}

typedef std::shared_ptr<Npc> NpcPtr;

}
