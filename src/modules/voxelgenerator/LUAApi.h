/**
 * @file
 */

#pragma once

#include "core/IComponent.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "command/CommandCompleter.h"
#include "io/Filesystem.h"
#include "noise/Noise.h"

struct lua_State;

namespace scenegraph {
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

class LUAApi : public core::IComponent {
private:
	noise::Noise _noise;
	io::FilesystemPtr _filesystem;
public:
	LUAApi(const io::FilesystemPtr &filesystem);
	virtual ~LUAApi() {}
	bool init() override;
	void update(double nowSeconds);
	void shutdown() override;

	core::String load(const core::String& scriptName) const;
	core::DynamicArray<LUAScript> listScripts() const;
	bool argumentInfo(const core::String& luaScript, core::DynamicArray<LUAParameterDescription>& params);
	/**
	 * @param luaScript The lua script string to execute
	 * @param sceneGraph The scene graph to operate on
	 * @param nodeId The node ID of the active node
	 * @param region The region to operate on
	 * @param voxel The voxel color and material that is currently selected
	 * @param dirtyRegion The region that was modified by the script
	 * @param args The arguments to pass to the script
	 * @return @c true if the script was executed successfully, @c false otherwise
	 */
	bool exec(const core::String &luaScript, scenegraph::SceneGraph &sceneGraph, int nodeId,
			  const voxel::Region &region, const voxel::Voxel &voxel, voxel::Region &dirtyRegion,
			  const core::DynamicArray<core::String> &args = {});
};

inline auto scriptCompleter(const io::FilesystemPtr& filesystem) {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return command::complete(filesystem, "scripts", str, matches, "*.lua");
	};
}

}
