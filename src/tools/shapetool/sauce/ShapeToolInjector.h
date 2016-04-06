#pragma once

#include "ShapeToolModule.h"

inline sauce::shared_ptr<sauce::Injector> getInjector() {
	sauce::Modules modules;
	modules.add<ShapeToolModule>();
	return modules.createInjector();
}
