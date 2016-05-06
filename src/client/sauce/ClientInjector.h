/**
 * @file
 */

#pragma once

#include "ClientModule.h"
#include "../network/ClientNetworkModule.h"

inline sauce::shared_ptr<sauce::Injector> getInjector() {
	sauce::Modules modules;
	modules.add<ClientModule>();
	modules.add<ClientNetworkModule>();
	return modules.createInjector();
}
