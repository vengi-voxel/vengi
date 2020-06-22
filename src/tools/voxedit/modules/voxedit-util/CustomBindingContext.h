/**
 * @file
 */

#pragma once

#include "core/BindingContext.h"

namespace voxedit {

enum BindingContext {
	UI = core::BindingContext::UserInterface,
	Scene = core::BindingContext::World,
};

}
