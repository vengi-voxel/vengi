/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"

struct lua_State;

namespace voxel {
class Region;
class RawVolumeWrapper;
class Voxel;
}

namespace voxelgenerator {

enum class LUAParameterType {
	String,
	Integer,
	Float,
	Boolean,

	Max
};

struct LUAParameterDescription {
	core::String name;
	core::String description;
	core::String defaultValue;
	LUAParameterType type;

	LUAParameterDescription(const core::String &_name, const core::String &_description, const core::String &_defaultValue, LUAParameterType _type)
		: name(_name), description(_description), defaultValue(_defaultValue), type(_type) {
	}
	LUAParameterDescription() : type(LUAParameterType::Max) {
	}
};

class LUAGenerator : public core::IComponent {
protected:
	virtual void initializeCustomState(lua_State* s) {}
public:
	virtual ~LUAGenerator() {}
	bool init() override;
	void shutdown() override;

	bool argumentInfo(const core::String& luaScript, core::DynamicArray<LUAParameterDescription>& params);
	bool exec(const core::String& luaScript, voxel::RawVolumeWrapper* volume, const voxel::Region& region, const voxel::Voxel& voxel, const core::DynamicArray<core::String>& args = core::DynamicArray<core::String>());
};

}
