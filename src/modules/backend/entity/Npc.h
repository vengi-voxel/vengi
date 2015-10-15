#pragma once

#include <SimpleAI.h>
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
	network::messages::NpcType _type;
	std::atomic_bool _humanControlled;
	voxel::WorldPtr _world;
	EntityStoragePtr _entityStorage;
	PoiProviderPtr _poiProvider;
	glm::ivec3 _homePosition;
	ai::AIPtr _ai;

	void moveToGround();

public:
	Npc(network::messages::NpcType type, const EntityStoragePtr& entityStorage, const ai::TreeNodePtr& behaviour, const voxel::WorldPtr& world, const network::MessageSenderPtr& messageSender,
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
	void releaseHumanControlled();
	bool aquireHumanControlled();
	bool humanControlled() const;
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

	glm::vec3 pos() const override;
	float orientation() const override;
	std::string name() const;

	network::messages::NpcType npcType() const override;
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

inline void Npc::releaseHumanControlled() {
	_humanControlled = false;
}

inline bool Npc::aquireHumanControlled() {
	bool expected = false;
	if (!_humanControlled.compare_exchange_strong(expected, true))
		return false;
	return true;
}

inline network::messages::NpcType Npc::npcType() const {
	return _type;
}

inline const ai::AIPtr& Npc::ai() {
	return _ai;
}

inline bool Npc::humanControlled() const {
	return _humanControlled;
}

typedef std::shared_ptr<Npc> NpcPtr;

}
