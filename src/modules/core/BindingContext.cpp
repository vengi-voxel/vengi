/**
 * @file
 */

#include "BindingContext.h"
#include "App.h"

namespace core {

BindingContext bindingContext() {
	return core::App::getInstance()->bindingContext();
}

BindingContext setBindingContext(int ctx) {
	return core::App::getInstance()->setBindingContext((core::BindingContext)ctx);
}

}
