/**
 * @file
 */

#include "MementoRedoTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

MementoRedoTool::MementoRedoTool() : Tool("voxedit_memento_redo") {
	_tool.set("description", "Redo the last undone action (optional n argument)");

	json::Json nprop = propTypeDescription("integer", "Number of redo steps");
	nprop.set("default", 1);

	json::Json in = json::Json::object();
	in.set("type", "object");
	in.get("properties").set("n", core::move(nprop));
	_tool.set("inputSchema", core::move(in));
}

bool MementoRedoTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const int n = args.intVal("n", 1);
	if (!ctx.sceneMgr->redo(n)) {
		return ctx.result(id, "Failed to redo", false);
	}
	return ctx.result(id, "Redo successful", false);
}

} // namespace voxedit
