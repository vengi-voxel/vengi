/**
 * @file
 */

#pragma once

#include "ShaderToolModule.h"

inline sauce::shared_ptr<sauce::Injector> getInjector() {
	sauce::Modules modules;
	modules.add<ShaderToolModule>();
	return modules.createInjector();
}
