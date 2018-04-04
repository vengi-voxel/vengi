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

	virtual void construct() {}
	virtual bool init() = 0;
	virtual void shutdown() = 0;
};

}
