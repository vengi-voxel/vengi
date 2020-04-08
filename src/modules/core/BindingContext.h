/**
 * @file
 */

#pragma once

namespace core {

/**
 * @brief Command bindings context
 *
 * @note Extend this in your own application by adding to @c BindingContext::FirstCustom if the available
 * types aren't enough for your use-cases.
 */
enum BindingContext {
	All,
	UserInterface,
	World,
	FirstCustom
};

/**
 * @brief Returns the current binding context
 */
extern BindingContext bindingContext();
/**
 * @brief Updates the current binding context and returns the old one
 */
extern BindingContext setBindingContext(int ctx);

}
