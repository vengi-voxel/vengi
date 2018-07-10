/**
 * @file
 */

#pragma once

#include "core/tests/AbstractTest.h"
#include "core/Var.h"
#include "core/GameConfig.h"

namespace persistence {

class AbstractDatabaseTest : public core::AbstractTest {
private:
	using Super = core::AbstractTest;
public:
	void SetUp() override {
		Super::SetUp();
		core::Var::get(cfg::DatabaseMinConnections, "1");
		core::Var::get(cfg::DatabaseMaxConnections, "2");
		core::Var::get(cfg::DatabaseName, "enginetest");
		core::Var::get(cfg::DatabaseHost, "localhost");
		core::Var::get(cfg::DatabaseUser, "engine");
		core::Var::get(cfg::DatabasePassword, "engine");
	}
};

}
