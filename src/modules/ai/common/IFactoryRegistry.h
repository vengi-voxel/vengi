/**
 * @file
 */
#pragma once

#include "common/NonCopyable.h"
#include <memory>
#include <map>

namespace ai {

template<class TYPE, class CTX>
class IFactory {
public:
	virtual ~IFactory() {}
	virtual std::shared_ptr<TYPE> create (const CTX* ctx) const = 0;
};

template<class KEY, class TYPE, class CTX>
class IFactoryRegistry: public NonCopyable {
protected:
	typedef std::map<const KEY, const IFactory<TYPE, CTX>*> FactoryMap;
	typedef typename FactoryMap::const_iterator FactoryMapConstIter;
	typedef typename FactoryMap::iterator FactoryMapIter;
	FactoryMap _factories;
public:
	bool registerFactory (const KEY& type, const IFactory<TYPE, CTX>& factory)
	{
		FactoryMapConstIter i = _factories.find(type);
		if (i != _factories.end()) {
			return false;
		}

		_factories[type] = &factory;
		return true;
	}

	bool unregisterFactory (const KEY& type)
	{
		FactoryMapIter i = _factories.find(type);
		if (i == _factories.end()) {
			return false;
		}

		_factories.erase(i);
		return true;
	}

	std::shared_ptr<TYPE> create (const KEY& type, const CTX* ctx = nullptr) const
	{
		FactoryMapConstIter i = _factories.find(type);
		if (i == _factories.end()) {
			return std::shared_ptr<TYPE>();
		}

		const IFactory<TYPE, CTX>* factory = i->second;

#if AI_EXCEPTIONS
		try {
#endif
			return factory->create(ctx);
#if AI_EXCEPTIONS
		} catch (...) {
		}
		return std::shared_ptr<TYPE>();
#endif
	}
};

}
