/**
 * @file
 */

#include "MementoCanRedoTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

MementoCanRedoTool::MementoCanRedoTool() : Tool("voxedit_memento_can_redo") {
	_tool["description"] = "Returns whether a redo operation is available";
	_tool["inputSchema"]["type"] = "object";
}

bool MementoCanRedoTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const memento::MementoHandler &mementoHandler = ctx.sceneMgr->mementoHandler();
	return ctx.result(id, mementoHandler.canRedo() ? "true" : "false", false);
}

} // namespace voxedit
