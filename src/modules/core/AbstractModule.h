/**
 * @file
 */

#pragma once

#include <sauce/sauce.h>
#include <utility>

#include "core/TimeProvider.h"
#include "core/EventBus.h"
#include "io/Filesystem.h"

namespace core {

class AbstractModule: public sauce::AbstractModule {
public:
	AbstractModule() : sauce::AbstractModule() {}
protected:
	template<class Class>
	inline void bindSingleton() const {
		typename sauce::BindClause<Class> bindClause = bind<Class>();
		typename sauce::InClause<sauce::Named<Class, sauce::Unnamed>, sauce::SingletonScope> inClause = bindClause.template in<sauce::SingletonScope>();
		inClause.template to<Class>();
	}

	void configure() const override {
		bindSingleton<core::TimeProvider>();
		bindSingleton<core::EventBus>();
		bindSingleton<io::Filesystem>();
	}
};

}
