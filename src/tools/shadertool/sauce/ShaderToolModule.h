/**
 * @file
 */

#pragma once

#include <sauce/sauce.h>

#include "core/AbstractModule.h"
#include "core/App.h"

class ShaderToolModule: public core::AbstractModule {
	void configure() const override {
		core::AbstractModule::configure();
		bind<ShaderTool>().in<sauce::SingletonScope>().to<ShaderTool(io::Filesystem &, core::EventBus &)>();
	}
};
