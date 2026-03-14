/**
 * @file
 */

#include "MementoCanUndoTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

MementoCanUndoTool::MementoCanUndoTool() : Tool("voxedit_memento_can_undo") {
	_tool.set("description", "Returns whether an undo operation is available");
	_tool.get("inputSchema").set("type", "object");
}

bool MementoCanUndoTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const memento::MementoHandler &mementoHandler = ctx.sceneMgr->mementoHandler();
	return ctx.result(id, mementoHandler.canUndo() ? "true" : "false", false);
}

} // namespace voxedit
