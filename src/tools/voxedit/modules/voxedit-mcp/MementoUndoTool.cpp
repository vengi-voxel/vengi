/**
 * @file
 */

#include "MementoUndoTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

MementoUndoTool::MementoUndoTool() : Tool("voxedit_memento_undo") {
	_tool.set("description", "Undo the last action (optional n argument)");

	json::Json nprop = propTypeDescription("integer", "Number of undo steps");
	nprop.set("default", 1);

	json::Json in = json::Json::object();
	in.set("type", "object");
	in.get("properties").set("n", core::move(nprop));
	_tool.set("inputSchema", core::move(in));
}

bool MementoUndoTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const int n = args.intVal("n", 1);
	if (!ctx.sceneMgr->undo(n)) {
		return ctx.result(id, "Failed to undo", false);
	}
	return ctx.result(id, "Undo successful", false);
}

} // namespace voxedit
