/**
 * @file
 */

#pragma once

#include "AbstractModule.h"
#include "App.h"

namespace core {

class AbstractAppModule: public AbstractModule {
protected:
	virtual void configureApp() const = 0;

	virtual void configureBindings() const = 0;

	virtual void configure() const override {
		AbstractModule::configure();
		configureBindings();
		configureApp();
	}
};

template<class AppClass>
class AppModule: public AbstractAppModule {
public:
	virtual ~AppModule() {
	}
protected:
	virtual void configureApp() const override {
		bind<AppClass>().template in<sauce::SingletonScope>().template to<AppClass(io::Filesystem &, core::EventBus &)>();
	}

	virtual void configureBindings() const override {
	}
};

template<typename AppClass, typename Module = AppModule<AppClass> >
inline typename sauce::shared_ptr<sauce::Injector> getAppInjector() {
	sauce::Modules modules;
	modules.template add<Module>();
	return modules.createInjector();
}

template<typename AppClass, typename Module = AppModule<AppClass> >
inline typename sauce::shared_ptr<AppClass> getApp() {
	return getAppInjector<AppClass, Module>()->template get<AppClass>();
}

inline void addModule(sauce::Modules& modules) {
}

template<typename Module, typename... Modules>
inline void addModule(sauce::Modules& modules, const Module& module, Modules&&... mods) {
	modules.add(module);
	addModule(modules, mods...);
}

template<typename AppClass, typename... Modules>
inline typename sauce::shared_ptr<AppClass> getAppWithModules(Modules&&... mods) {
	sauce::Modules modules;
	addModule(modules, mods...);
	return modules.createInjector()->template get<AppClass>();
}

}
