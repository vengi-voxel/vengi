/**
 * @file
 */

#pragma once

#include "NoiseToolModule.h"

inline sauce::shared_ptr<sauce::Injector> getInjector() {
	sauce::Modules modules;
	modules.add<NoiseToolModule>();
	return modules.createInjector();
}
