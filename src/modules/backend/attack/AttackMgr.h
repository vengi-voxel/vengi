/**
 * @file
 */

#pragma once

#include "backend/entity/EntityId.h"
#include "core/IComponent.h"

namespace backend {

class Map;

/**
 * @brief Manages the attacks on a map
 */
class AttackMgr : public core::IComponent {
private:
	Map* _map;
public:
	AttackMgr(Map* map);

	bool init() override;
	/**
	 * @brief Stops all running attacks
	 */
	void shutdown() override;

	/**
	 * @brief Executes the running attacks.
	 */
	void update(long dt);

	/**
	 * @brief Uses the current selected weapon to attack the victim
	 * @return @c false If the attack could not start because the victim is not
	 * known on the map where the attacker is or the current selected weapon
	 * can't be used to attack the victim.
	 */
	bool startAttack(EntityId attackerId, EntityId victimId);

	/**
	 * @brief Stops the attack on the given victim.
	 * @return @c false If no such attack exists or it can't get aborted.
	 */
	bool stopAttack(EntityId attackerId, EntityId victimId);
};

}
