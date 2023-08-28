/**
 * @file
 */

#pragma once

#include "Brush.h"
#include "voxelgenerator/LUAGenerator.h"

namespace voxedit {

class ScriptBrush : public Brush {
private:
	using Super = Brush;

protected:
	voxelgenerator::LUAGenerator _luaGenerator;
	core::String _luaCode;
	core::DynamicArray<core::String> _args;

public:
	bool execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
				 const BrushContext &context) override;
	void setScriptCode(const core::String &luaCode, const core::DynamicArray<core::String> &args);

	bool init() override;
	void shutdown() override;
	voxelgenerator::LUAGenerator &luaGenerator();
};

inline voxelgenerator::LUAGenerator &ScriptBrush::luaGenerator() {
	return _luaGenerator;
}

inline void ScriptBrush::setScriptCode(const core::String &luaCode, const core::DynamicArray<core::String> &args) {
	_luaCode = luaCode;
	_args = args;
}

} // namespace voxedit
