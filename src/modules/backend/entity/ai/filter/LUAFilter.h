/**
 * @file
 * @ingroup LUA
 * @ingroup Filter
 */
#pragma once

#include "IFilter.h"
#include "commonlua/LUA.h"

namespace backend {

/**
 * @see @ai{LUAAIRegistry}
 */
class LUAFilter : public IFilter {
protected:
	lua_State* _s;

	void filterLUA(const AIPtr& entity);

public:
	class LUAFilterFactory : public IFilterFactory {
	private:
		lua_State* _s;
		core::String _type;
	public:
		LUAFilterFactory(lua_State* s, const core::String& typeStr) :
				_s(s), _type(typeStr) {
		}

		inline const core::String& type() const {
			return _type;
		}

		FilterPtr create(const FilterFactoryContext* ctx) const override {
			return std::make_shared<LUAFilter>(_type, ctx->parameters, _s);
		}
	};

	LUAFilter(const core::String& name, const core::String& parameters, lua_State* s) :
			IFilter(name, parameters), _s(s) {
	}

	~LUAFilter() {
	}

	void filter(const AIPtr& entity) override {
		filterLUA(entity);
	}
};

}
