/**
 * @file
 */

#include "BindingContext.h"

namespace core {

namespace _priv {
static BindingContext _bindingContext = BindingContext::All;
}

BindingContext bindingContext() {
	return _priv::_bindingContext;
}

BindingContext setBindingContext(int ctx) {
	const core::BindingContext newContext = (core::BindingContext)ctx;
	if (_priv::_bindingContext == newContext) {
		return newContext;
	}
	const core::BindingContext old = _priv::_bindingContext;
	_priv::_bindingContext = newContext;
	return old;
}

}
