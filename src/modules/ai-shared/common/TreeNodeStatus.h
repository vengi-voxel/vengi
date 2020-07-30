/**
 * @file
 */
#pragma once

#include <stdint.h>

namespace ai {

/**
 * @brief Execution states of a TreeNode::execute() call
 */
enum class TreeNodeStatus : uint8_t {
	UNKNOWN,
	/**
	 * Not every condition is met in order to run this node
	 * In general this means that the attached condition has to evaluate to @c true
	 */
	CANNOTEXECUTE,
	/**
	 * This behavior is still running and thus can block others
	 */
	RUNNING,
	/**
	 * The behavior ran and terminated without any problems.
	 */
	FINISHED,
	/**
	 * Controlled failure
	 */
	FAILED,
	/**
	 * Unexpected failure while executing the the node's action
	 */
	EXCEPTION,

	MAX_TREENODESTATUS
};

}
