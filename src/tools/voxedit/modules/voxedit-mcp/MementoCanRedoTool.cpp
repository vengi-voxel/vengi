/**
 * @file
 */

#include "MementoCanRedoTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

MementoCanRedoTool::MementoCanRedoTool() : Tool("voxedit_memento_can_redo") {
	_tool.set("description", "Returns whether a redo operation is available");
	_tool.get("inputSchema").set("type", "object");
}

bool MementoCanRedoTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const memento::MementoHandler &mementoHandler = ctx.sceneMgr->mementoHandler();
	return ctx.result(id, mementoHandler.canRedo() ? "true" : "false", false);
}

} // namespace voxedit
