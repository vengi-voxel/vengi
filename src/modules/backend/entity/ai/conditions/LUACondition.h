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

	bool evaluateLUA(const AIPtr& entity) {
		// get userdata of the condition
		const core::String name = "__meta_condition_" + _name;
		lua_getfield(_s, LUA_REGISTRYINDEX, name.c_str());
#if AI_LUA_SANTITY > 0
		if (lua_isnil(_s, -1)) {
			Log::error("LUA condition: could not find lua userdata for %s", _name.c_str());
			return false;
		}
#endif
		// get metatable
		lua_getmetatable(_s, -1);
#if AI_LUA_SANTITY > 0
		if (!lua_istable(_s, -1)) {
			Log::error("LUA condition: userdata for %s doesn't have a metatable assigned", _name.c_str());
			return false;
		}
#endif
		// get evaluate() method
		lua_getfield(_s, -1, "evaluate");
		if (!lua_isfunction(_s, -1)) {
			Log::error("LUA condition: metatable for %s doesn't have the evaluate() function assigned", _name.c_str());
			return false;
		}

		// push self onto the stack
		lua_getfield(_s, LUA_REGISTRYINDEX, name.c_str());

		// first parameter is ai
		if (luaAI_pushai(_s, entity) == 0) {
			return false;
		}

#if AI_LUA_SANTITY > 0
		if (!lua_isfunction(_s, -3)) {
			Log::error("LUA condition: expected to find a function on stack -3");
			return false;
		}
		if (!lua_isuserdata(_s, -2)) {
			Log::error("LUA condition: expected to find the userdata on -2");
			return false;
		}
		if (!lua_isuserdata(_s, -1)) {
			Log::error("LUA condition: second parameter should be the ai");
			return false;
		}
#endif
		const int error = lua_pcall(_s, 2, 1, 0);
		if (error) {
			Log::error("LUA condition script: %s", lua_isstring(_s, -1) ? lua_tostring(_s, -1) : "Unknown Error");
			// reset stack
			lua_pop(_s, lua_gettop(_s));
			return false;
		}
		const int state = lua_toboolean(_s, -1);
		if (state != 0 && state != 1) {
			Log::error("LUA condition: illegal evaluate() value returned: %i", state);
			return false;
		}

		// reset stack
		lua_pop(_s, lua_gettop(_s));
		return state == 1;
	}

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
