/**
 * @file
 */

#pragma once

#include <stdint.h>

namespace core {

class String;

/**
 * @brief Command bindings context
 */
enum BindingContext : uint8_t {
	All = 0,
	UI = 1,
	Context1 = 2,
	Context2 = 4,
	Context3 = 8,
	Context4 = 16,
	// if this flag is set to the key binding context, it is only active if the current binding context matches exactly
	// and no other context is set
	ContextExclusive = 128,
};

/**
 * @brief Get the current binding context
 * @sa setBindingContext() for more details.
 */
BindingContext bindingContext();
/**
 * @brief Allows to change the binding context. This can be used to e.g. ignore some commands while hovering
 * the ui and they should only be active if the scene has the focus.
 * @return the old context
 * @sa bindingContext()
 */
BindingContext setBindingContext(int ctx);
BindingContext parseBindingContext(const String &context);
core::String bindingContextString(int ctx);
bool registerBindingContext(const String &context, int ctx);
void resetBindingContexts();

inline bool isSuitableBindingContext(core::BindingContext context) {
	if (context == core::BindingContext::All || core::bindingContext() == core::BindingContext::All) {
		return true;
	}
	if (context & ContextExclusive) {
		return core::bindingContext() == (context & ~ContextExclusive);
	}
	return (core::bindingContext() & context) != 0;
}

class ScopedBindingContext {
private:
	const BindingContext _oldContext;
public:
	ScopedBindingContext(BindingContext newContext) : _oldContext(core::setBindingContext(newContext)) {
	}
	~ScopedBindingContext() {
		core::setBindingContext(_oldContext);
	}
};

}
