/**
 * @file
 * @ingroup Condition
 * @ingroup LUA
 */
#pragma once

#include "ICondition.h"
#include "backend/entity/ai/LUAFunctions.h"

namespace backend {

/**
 * @see @ai{LUAAIRegistry}
 */
class LUACondition : public ICondition {
protected:
	lua_State* _s;

	bool evaluateLUA(const AIPtr& entity);

public:
	class LUAConditionFactory : public IConditionFactory {
	private:
		lua_State* _s;
		core::String _type;
	public:
		LUAConditionFactory(lua_State* s, const core::String& typeStr) :
				_s(s), _type(typeStr) {
		}

		inline const core::String& type() const {
			return _type;
		}

		ConditionPtr create(const ConditionFactoryContext* ctx) const override {
			return std::make_shared<LUACondition>(_type, ctx->parameters, _s);
		}
	};

	LUACondition(const core::String& name, const core::String& parameters, lua_State* s) :
			ICondition(name, parameters), _s(s) {
	}

	~LUACondition() {
	}

	bool evaluate(const AIPtr& entity) override {
		return evaluateLUA(entity);
	}
};

}
