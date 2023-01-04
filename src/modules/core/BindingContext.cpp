/**
 * @file
 */

#include "BindingContext.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/collection/Array.h"
#include "core/collection/DynamicArray.h"

namespace core {

namespace _priv {
static BindingContext _bindingContext = BindingContext::All;
static core::Array<core::String, 32> _registeredBindingContext;
}

BindingContext bindingContext() {
	return _priv::_bindingContext;
}

const core::String& bindingContextString(int ctx) {
	if (ctx < 0 || ctx >= (int)_priv::_registeredBindingContext.size()) {
		return _priv::_registeredBindingContext[0];
	}
	return _priv::_registeredBindingContext[ctx];
}

BindingContext parseBindingContext(const String &context) {
	for (size_t i = 0; i < _priv::_registeredBindingContext.size(); ++i) {
		if (_priv::_registeredBindingContext[i] == context) {
			return (BindingContext)i;
		}
	}
	Log::warn("Could not find a registered binding context for '%s'", context.c_str());
	return BindingContext::All;
}

bool registerBindingContext(const String &context, int ctx) {
	if (ctx < 0 || ctx >= (int)_priv::_registeredBindingContext.size()) {
		Log::error("Failed to register binding context - %i out of bounds", ctx);
		return false;
	}
	_priv::_registeredBindingContext[ctx] = context;
	return true;
}

void resetBindingContexts() {
	for (core::String &c : _priv::_registeredBindingContext) {
		c = "";
	}
}

BindingContext setBindingContext(int ctx) {
	const core::BindingContext newContext = (core::BindingContext)ctx;
	const core::BindingContext old = _priv::_bindingContext;
	_priv::_bindingContext = newContext;
	return old;
}

}
