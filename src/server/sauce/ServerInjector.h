/**
 * @file
 */

#pragma once

#include "ServerModule.h"
#include "backend/network/ServerNetworkModule.h"

inline sauce::shared_ptr<sauce::Injector> getInjector() {
	sauce::Modules modules;
	modules.add<ServerModule>();
	modules.add<backend::ServerNetworkModule>();
	return modules.createInjector();
}
