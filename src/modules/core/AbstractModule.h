#pragma once

#include <sauce/sauce.h>

#include "core/TimeProvider.h"
#include "core/EventBus.h"
#include "io/Filesystem.h"

namespace core {

class AbstractModule: public sauce::AbstractModule {
protected:
	void configure() const override {
		bind<core::TimeProvider>().in<sauce::SingletonScope>().to<core::TimeProvider>();
		bind<core::EventBus>().in<sauce::SingletonScope>().to<core::EventBus>();
		bind<io::Filesystem>().in<sauce::SingletonScope>().to<io::Filesystem>();
	}
};

}
