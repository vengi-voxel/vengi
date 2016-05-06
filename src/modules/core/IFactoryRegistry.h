/**
 * @file
 */

#pragma once

#include <unordered_map>
#include <memory>

#include "NonCopyable.h"

namespace core {

template<class TYPE, class CTX>
class IFactory {
public:
	virtual ~IFactory() {
	}
	virtual std::shared_ptr<TYPE> create(const CTX* ctx) const = 0;
};

template<class KEY, class TYPE, class CTX>
class IFactoryRegistry: public NonCopyable {
public:
	typedef std::unordered_map<const KEY*, const IFactory<TYPE, CTX>*> FactoryMap;
	typedef typename FactoryMap::const_iterator FactoryMapConstIter;
	typedef typename FactoryMap::iterator FactoryMapIter;
protected:
	FactoryMap _factories;
public:
	bool registerFactory(const KEY& type, const IFactory<TYPE, CTX>& factory) {
		FactoryMapConstIter i = _factories.find(&type);
		if (i != _factories.end()) {
			return false;
		}

		_factories[&type] = &factory;
		return true;
	}

	std::shared_ptr<TYPE> create(const KEY& type, const CTX* ctx = 0) const {
		FactoryMapConstIter i = _factories.find(&type);
		if (i == _factories.end()) {
			return std::shared_ptr<TYPE>();
		}

		const IFactory<TYPE, CTX>* factory = i->second;
		return factory->create(ctx);
	}

	inline const FactoryMap& getFactories() const {
		return _factories;
	}
};

}
