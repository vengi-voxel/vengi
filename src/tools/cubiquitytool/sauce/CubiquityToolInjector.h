#pragma once

#include "CubiquityToolModule.h"

inline sauce::shared_ptr<sauce::Injector> getInjector() {
	sauce::Modules modules;
	modules.add<CubiquityToolModule>();
	return modules.createInjector();
}
