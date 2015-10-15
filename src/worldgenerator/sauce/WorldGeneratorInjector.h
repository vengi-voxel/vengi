#pragma once

#include "WorldGeneratorModule.h"

inline sauce::shared_ptr<sauce::Injector> getInjector() {
	sauce::Modules modules;
	modules.add<WorldGeneratorModule>();
	return modules.createInjector();
}
