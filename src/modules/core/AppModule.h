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
#ifdef DI_SAUCE
		bind<AppClass>().template in<di::SingletonScope>().template to<AppClass(io::Filesystem &, core::EventBus &)>();
#endif
#ifdef DI_BOOST
		bindSingleton<AppClass>();
#endif
	}

	virtual void configureBindings() const override {
	}
};

template<typename AppClass, typename Module = AppModule<AppClass> >
inline typename std::shared_ptr<di::Injector> getAppInjector() {
	di::Modules modules;
	modules.template add<Module>();
	return modules.createInjector();
}

template<typename AppClass, typename Module = AppModule<AppClass> >
inline typename std::shared_ptr<AppClass> getApp() {
	return getAppInjector<AppClass, Module>()->template get<AppClass>();
}

inline void addModule(di::Modules& modules) {
}

template<typename Module, typename... Modules>
inline void addModule(di::Modules& modules, const Module& module, Modules&&... mods) {
	modules.add(module);
	addModule(modules, mods...);
}

template<typename AppClass, typename... Modules>
inline typename std::shared_ptr<AppClass> getAppWithModules(Modules&&... mods) {
	di::Modules modules;
	addModule(modules, mods...);
	return modules.createInjector()->template get<AppClass>();
}

}
