/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "command/CommandCompleter.h"
#include "noise/Noise.h"

struct lua_State;

namespace voxelformat {
class SceneGraph;
}

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
	Enum,

	Max
};

struct LUAParameterDescription {
	core::String name;
	core::String description;
	core::String defaultValue;
	core::String enumValues; /**< separated by ; */
	double minValue = 1.0;
	double maxValue = 0.0;
	LUAParameterType type;

	LUAParameterDescription(const core::String &_name, const core::String &_description, const core::String &_defaultValue, const core::String &_enumValues, double _minValue, double _maxValue, LUAParameterType _type)
		: name(_name), description(_description), defaultValue(_defaultValue), enumValues(_enumValues), minValue(_minValue), maxValue(_maxValue), type(_type) {
	}
	LUAParameterDescription() : type(LUAParameterType::Max) {
	}

	inline bool shouldClamp() const {
		return minValue < maxValue;
	}
};

struct LUAScript {
	core::String filename;
	bool valid; // main() was found

	const char *c_str() const {
		return filename.c_str();
	}
};

class LUAGenerator : public core::IComponent {
private:
	noise::Noise _noise;
public:
	virtual ~LUAGenerator() {}
	bool init() override;
	void shutdown() override;

	core::String load(const core::String& scriptName) const;
	core::DynamicArray<LUAScript> listScripts() const;
	bool argumentInfo(const core::String& luaScript, core::DynamicArray<LUAParameterDescription>& params);
	bool exec(const core::String& luaScript, voxelformat::SceneGraph &sceneGraph, int nodeId, const voxel::Region& region, const voxel::Voxel& voxel, voxel::Region &dirtyRegion, const core::DynamicArray<core::String>& args = {});
};

inline auto scriptCompleter(const io::FilesystemPtr& filesystem) {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return command::complete(filesystem, "scripts", str, matches, "*.lua");
	};
}

}
