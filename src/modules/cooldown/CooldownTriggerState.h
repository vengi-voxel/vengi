/**
 * @file
 */

#pragma once

namespace cooldown {

/**
 * @brief The cooldown trigger states
 * @ingroup Cooldowns
 */
enum CooldownTriggerState {
	/**
	 * @brief Cooldown was successfully triggered and is running now.
	 */
	SUCCESS,
	/**
	 * @brief There is already a cooldown of the same type running.
	 */
	ALREADY_RUNNING,
	FAILED
};

}
