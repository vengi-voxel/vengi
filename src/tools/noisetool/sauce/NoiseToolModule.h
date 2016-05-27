/**
 * @file
 */

#pragma once

#include <sauce/sauce.h>

#include "core/AbstractModule.h"
#include "core/App.h"

class NoiseToolModule: public core::AbstractModule {
	void configure() const override {
		core::AbstractModule::configure();
		bind<NoiseTool>().in<sauce::SingletonScope>().to<NoiseTool(io::Filesystem &, core::EventBus &)>();
	}
};
