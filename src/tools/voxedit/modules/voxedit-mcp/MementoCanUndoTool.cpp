/**
 * @file
 */

#include "MementoCanUndoTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

MementoCanUndoTool::MementoCanUndoTool() : Tool("voxedit_memento_can_undo") {
	_tool["description"] = "Returns whether an undo operation is available";
	_tool["inputSchema"]["type"] = "object";
}

bool MementoCanUndoTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const memento::MementoHandler &mementoHandler = ctx.sceneMgr->mementoHandler();
	return ctx.result(id, mementoHandler.canUndo() ? "true" : "false", false);
}

} // namespace voxedit
