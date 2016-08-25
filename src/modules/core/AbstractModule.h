/**
 * @file
 */

#pragma once

#include "DependencyInjection.h"
#include <utility>

#include "core/TimeProvider.h"
#include "core/EventBus.h"
#include "io/Filesystem.h"

namespace core {

#ifdef DI_SAUCE
class AbstractModule: public di::AbstractModule {
public:
	AbstractModule() : di::AbstractModule() {}
protected:
	template<class Class>
	inline void bindSingleton() const {
		typename di::BindClause<Class> bindClause = bind<Class>();
		typename di::InClause<di::Named<Class, di::Unnamed>, sauce::SingletonScope> inClause = bindClause.template in<di::SingletonScope>();
		inClause.template to<Class>();
	}

	void configure() const override {
		bindSingleton<core::TimeProvider>();
		bindSingleton<core::EventBus>();
		bindSingleton<io::Filesystem>();
	}
};
#endif

#ifdef DI_BOOST

class AbstractModule {
protected:
	AbstractModule() {
		auto _injector = di::make_injector();
	}

	virtual ~AbstractModule() {}

	template<class Class>
	inline void bindSingleton() const {
	}

	template<class Class>
	inline auto bind() const {
		return di::bind<Class>();
	}

	virtual void configure() const {
		bindSingleton<core::TimeProvider>();
		bindSingleton<core::EventBus>();
		bindSingleton<io::Filesystem>();
	}
};
#endif

}
