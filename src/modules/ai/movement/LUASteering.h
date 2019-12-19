/**
 * @file
 * @ingroup LUA
 */
#pragma once

#include "Steering.h"
#include "commonlua/LUA.h"

namespace ai {
namespace movement {

/**
 * @see @ai{LUAAIRegistry}
 */
class LUASteering : public ISteering {
protected:
	mutable lua_State* _s;
	std::string _type;

	MoveVector executeLUA(const AIPtr& entity, float speed) const;

public:
	class LUASteeringFactory : public ISteeringFactory {
	private:
		lua_State* _s;
		std::string _type;
	public:
		LUASteeringFactory(lua_State* s, const std::string& typeStr) :
				_s(s), _type(typeStr) {
		}

		inline const std::string& type() const {
			return _type;
		}

		SteeringPtr create(const SteeringFactoryContext* ctx) const override {
			return std::make_shared<LUASteering>(_s, _type);
		}
	};

	LUASteering(lua_State* s, const std::string& type);

	~LUASteering() {
	}

	MoveVector execute(const AIPtr& entity, float speed) const override;
};

}
}
