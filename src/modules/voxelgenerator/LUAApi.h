/**
 * @file
 */

#pragma once

#include "command/CommandCompleter.h"
#include "commonlua/LUA.h"
#include "core/IComponent.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"
#include "noise/Noise.h"
#include "voxel/Region.h"

struct lua_State;

namespace scenegraph {
class SceneGraph;
}

namespace voxel {
class RawVolumeWrapper;
class Voxel;
} // namespace voxel

namespace voxelgenerator {

enum class LUAParameterType {
	String,
	Integer,
	Float,
	Boolean,
	ColorIndex,
	Enum,
	File,

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

	LUAParameterDescription(const core::String &_name, const core::String &_description,
							const core::String &_defaultValue, const core::String &_enumValues, double _minValue,
							double _maxValue, LUAParameterType _type)
		: name(_name), description(_description), defaultValue(_defaultValue), enumValues(_enumValues),
		  minValue(_minValue), maxValue(_maxValue), type(_type) {
	}
	LUAParameterDescription() : type(LUAParameterType::Max) {
	}

	inline bool shouldClamp() const {
		return minValue < maxValue;
	}
};

struct LUAScript {
	core::String filename;
	bool valid = false; // main() was found
	bool cached = false;
	core::String desc;

	core::DynamicArray<voxelgenerator::LUAParameterDescription> parameterDescription;
	core::DynamicArray<core::String> enumValues;
	core::DynamicArray<core::String> parameters;

	const char *c_str() const {
		return filename.c_str();
	}
};

enum class ScriptState { Running, Finished, Inactive, Error };

class LUAApi : public core::IComponent {
private:
	noise::Noise _noise;
	io::FilesystemPtr _filesystem;
	lua::LUA _lua;
	core::DynamicArray<LUAParameterDescription> _argsInfo;
	voxel::Region _dirtyRegion = voxel::Region::InvalidRegion;
	bool _scriptStillRunning = false;
	int _nargs = 0;

public:
	LUAApi(const io::FilesystemPtr &filesystem);
	virtual ~LUAApi() {
	}
	bool init() override;
	ScriptState update(double nowSeconds);
	void shutdown() override;

	void reloadScriptParameters(voxelgenerator::LUAScript &s);
	void reloadScriptParameters(voxelgenerator::LUAScript &s, const core::String &luaScript);

	bool scriptStillRunning() const {
		return _scriptStillRunning;
	}

	const core::String &error() const;

	core::String load(const core::String &scriptName) const;
	bool prepare(lua::LUA &lua, const core::String &luaScript) const;
	core::DynamicArray<LUAScript> listScripts() const;
	bool argumentInfo(lua::LUA& lua, core::DynamicArray<LUAParameterDescription> &params);
	core::String description(lua::LUA& lua) const;
	bool argumentInfo(const core::String &luaScript, core::DynamicArray<LUAParameterDescription> &params);
	core::String description(const core::String &luaScript) const;
	/**
	 * @note The real execution happens in the @c update() method
	 * @param luaScript The lua script string to execute
	 * @param sceneGraph The scene graph to operate on - this is the active scene graph and a pointer is stored until @c
	 * update() returned @c ScriptState::Finished
	 * @param nodeId The node ID of the active node
	 * @param region The region to operate on
	 * @param voxel The voxel color and material that is currently selected
	 * @param args The arguments to pass to the script
	 * @return @c true if the script was scheduled successfully, @c false otherwise
	 */
	bool exec(const core::String &luaScript, scenegraph::SceneGraph &sceneGraph, int nodeId,
			  const voxel::Region &region, const voxel::Voxel &voxel,
			  const core::DynamicArray<core::String> &args = {});

	const voxel::Region &dirtyRegion() const;

	/**
	 * @brief Generate a JSON representation of the Lua API
	 * @return JSON string containing all registered globals and metatables with their methods
	 */
	core::String apiJson() const;
};

inline const core::String &LUAApi::error() const {
	return _lua.error();
}

inline const voxel::Region &LUAApi::dirtyRegion() const {
	return _dirtyRegion;
}

inline auto scriptCompleter(const io::FilesystemPtr &filesystem) {
	return [=](const core::String &str, core::DynamicArray<core::String> &matches) -> int {
		return command::complete(filesystem, "scripts", str, matches, "*.lua");
	};
}

} // namespace voxelgenerator
