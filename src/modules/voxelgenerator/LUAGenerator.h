/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "core/IComponent.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "command/CommandCompleter.h"

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
	ColorIndex,

	Max
};

struct LUAParameterDescription {
	core::String name;
	core::String description;
	core::String defaultValue;
	double minValue = 0.0;
	double maxValue = 100.0;
	LUAParameterType type;

	LUAParameterDescription(const core::String &_name, const core::String &_description, const core::String &_defaultValue, double _minValue, double _maxValue, LUAParameterType _type)
		: name(_name), description(_description), defaultValue(_defaultValue), minValue(_minValue), maxValue(_maxValue), type(_type) {
	}
	LUAParameterDescription() : type(LUAParameterType::Max) {
	}
};

class LUAGenerator : public core::IComponent {
protected:
	virtual void initializeCustomState(lua_State* s) {}
public:
	static inline const char *luaVoxel_metaregion() {
		return "__meta_region";
	}

	static int luaVoxel_pushregion(lua_State* s, const voxel::Region* region);
	static voxel::Region* luaVoxel_toRegion(lua_State* s, int n);

	virtual ~LUAGenerator() {}
	bool init() override;
	void shutdown() override;

	core::String load(const core::String& scriptName) const;
	core::DynamicArray<core::String> listScripts() const;
	bool argumentInfo(const core::String& luaScript, core::DynamicArray<LUAParameterDescription>& params);
	bool exec(const core::String& luaScript, voxel::RawVolumeWrapper* volume, const voxel::Region& region, const voxel::Voxel& voxel, const core::DynamicArray<core::String>& args = core::DynamicArray<core::String>());
};

inline auto scriptCompleter(const io::FilesystemPtr& filesystem) {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return command::complete(filesystem, "scripts", str, matches, "*.lua");
	};
}

}
