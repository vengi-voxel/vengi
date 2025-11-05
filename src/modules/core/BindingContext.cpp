/**
 * @file
 */

#include "BindingContext.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/collection/Array.h"

namespace core {

namespace _priv {
static BindingContext _bindingContext = BindingContext::All;
static core::Array<core::String, 32> _registeredBindingContext;
}

BindingContext bindingContext() {
	return _priv::_bindingContext;
}

core::String bindingContextString(int ctx) {
	const bool exclusive = (ctx & ContextExclusive) != 0;
	ctx &= ~ContextExclusive;
	if (ctx < 0 || ctx >= (int)_priv::_registeredBindingContext.size()) {
		if (exclusive) {
			return core::String::format("!%s", _priv::_registeredBindingContext[0].c_str());
		}
		return _priv::_registeredBindingContext[0];
	}
	if (exclusive) {
		return core::String::format("!%s", _priv::_registeredBindingContext[ctx].c_str());
	}
	return _priv::_registeredBindingContext[ctx];
}

BindingContext parseBindingContext(const String &context) {
	if (context[0] == '!') {
		const core::String ctx = context.substr(1);
		for (size_t i = 0; i < _priv::_registeredBindingContext.size(); ++i) {
			if (_priv::_registeredBindingContext[i] == ctx) {
				return (BindingContext)(i | ContextExclusive);
			}
		}
		Log::warn("Could not find a registered binding context for '%s'", context.c_str());
		return BindingContext::All;
	}
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
