/**
 * @file
 */

#pragma once

#include "core/BindingContext.h"

namespace voxedit {

enum BindingContext {
	UI = core::BindingContext::FirstCustom + 0,
	Scene = core::BindingContext::FirstCustom + 1,
};

}
