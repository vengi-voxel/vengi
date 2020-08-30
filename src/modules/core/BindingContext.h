/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace core {

/**
 * @brief Command bindings context
 *
 * @note Extend this in your own application by adding to @c BindingContext::FirstCustom if the available
 * types aren't enough for your use-cases.
 */
enum BindingContext : uint8_t {
	All = 0,
	UserInterface = 1,
	World = 2,
	FirstCustom = 4
};

/**
 * @brief Get the current binding context
 * @sa setBindingContext() for more details.
 */
extern BindingContext bindingContext();
/**
 * @brief Allows to change the binding context. This can be used to e.g. ignore some commands while hovering
 * the ui and they should only be active if the scene has the focus.
 * @return the old context
 * @sa bindingContext()
 */
extern BindingContext setBindingContext(int ctx);

}
