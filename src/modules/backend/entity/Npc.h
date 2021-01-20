/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "ai-shared/common/CharacterId.h"
#include "backend/entity/Entity.h"
#include "cooldown/CooldownMgr.h"
#include "backend/ForwardDecl.h"
#include "backend/entity/EntityId.h"
#include "backend/network/ServerMessageSender.h"
#include "math/Random.h"

#include <atomic>

namespace backend {

/**
 * @brief AI controlled @c Entity
 */
class Npc: public Entity {
private:
	using Super = Entity;
	friend class AICharacter;
	static std::atomic<EntityId> _nextNpcId;
	glm::vec3 _homePosition;
	glm::vec3 _targetPosition;
	AIPtr _ai;
	AICharacterPtr _aiChr;

	// cooldowns
	cooldown::CooldownMgr _cooldowns;

	void moveToGround();

	// transfer from ai to npc state
	void updateFromAIState();
	// transfer to ai state
	void updateAIState();

	void init() override;

public:
	Npc(network::EntityType type,
			const TreeNodePtr& behaviour,
			const MapPtr& map,
			const network::ServerMessageSenderPtr& messageSender,
			const core::TimeProviderPtr& timeProvider,
			const attrib::ContainerProviderPtr& containerProvider,
			const cooldown::CooldownProviderPtr& cooldownProvider);
	~Npc();

	void init(const glm::ivec3* pos);
	void shutdown() override;

	void setHomePosition(const glm::vec3& pos);
	const glm::vec3& homePosition() const;
	void setTargetPosition(const glm::vec3& pos);
	const glm::vec3& targetPosition() const;
	bool route(const glm::vec3& target);
	const AIPtr& ai();

	cooldown::CooldownMgr& cooldownMgr();

	bool die();
	/**
	 * @brief Applies damage to the entity
	 * @param attacker The attacking @c Entity. This might be @c nullptr
	 * @param[in] damage The damage that the attacker tries to apply to the victim.
	 * @return The amount of applied damage.
	 * @note The amount of the applied damage might be less that the intended damage
	 * @note The victim gets aggro on the attacker
	 */
	double applyDamage(Entity* attacker, double damage);

	bool update(long dt) override;
};

inline void Npc::setHomePosition(const glm::vec3& pos) {
	_homePosition = pos;
}

inline const glm::vec3& Npc::homePosition() const {
	return _homePosition;
}

inline void Npc::setTargetPosition(const glm::vec3& pos) {
	_targetPosition = pos;
}

inline const glm::vec3& Npc::targetPosition() const {
	return _targetPosition;
}

inline const AIPtr& Npc::ai() {
	return _ai;
}

inline cooldown::CooldownMgr& Npc::cooldownMgr() {
	return _cooldowns;
}

typedef std::shared_ptr<Npc> NpcPtr;

}
