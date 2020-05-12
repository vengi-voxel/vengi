/**
 * @file
 */

#pragma once

#include "core/NonCopyable.h"

namespace core {

/**
 * @brief Interface that models the life-cycle of a component.
 */
struct IComponent : public core::NonCopyable {
	virtual ~IComponent() {}

	/**
	 * @brief Register commands and cvars before @c init() is called
	 * @note This is useful to be able to print all commands and cvars with
	 * e.g. @c --help command line parameter
	 */
	virtual void construct() {}
	/**
	 * @brief Called after @c construct()
	 * @return Returns @c true if the initialization was successful. On @c false
	 * @c shutdown() is called, too
	 */
	virtual bool init() = 0;
	/**
	 * @brief Called on restarting or shutting down
	 */
	virtual void shutdown() = 0;
};

}
