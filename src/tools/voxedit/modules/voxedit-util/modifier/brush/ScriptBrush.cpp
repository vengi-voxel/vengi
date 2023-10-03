/**
 * @file
 */

#include "ScriptBrush.h"
#include "core/Log.h"
#include "scenegraph/SceneGraph.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxel/Region.h"

namespace voxedit {

bool ScriptBrush::execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper,
						  const BrushContext &context) {
	if (_luaCode.empty()) {
		Log::warn("No script selected");
		return false;
	}
	const int nodeId = sceneGraph.activeNode();
	// TODO: context.lockedAxis support
	voxel::Region dirtyRegion = voxel::Region::InvalidRegion;
	bool state =
		_luaGenerator.exec(_luaCode, sceneGraph, nodeId, wrapper.region(), context.cursorVoxel, dirtyRegion, _args);
	wrapper.addDirtyRegion(dirtyRegion);
	return state;
}

bool ScriptBrush::init() {
	if (!Super::init()) {
		return false;
	}
	return _luaGenerator.init();
}

void ScriptBrush::shutdown() {
	_luaGenerator.shutdown();
	Super::shutdown();
}

} // namespace voxedit
