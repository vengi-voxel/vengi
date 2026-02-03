/**
 * @file
 */

#include "MementoUndoTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

MementoUndoTool::MementoUndoTool() : Tool("voxedit_memento_undo") {
	_tool["description"] = "Undo the last action (optional n argument)";

	nlohmann::json nprop = propTypeDescription("integer", "Number of undo steps");
	nprop["default"] = 1;

	nlohmann::json in;
	in["type"] = "object";
	in["properties"]["n"] = core::move(nprop);
	_tool["inputSchema"] = core::move(in);
}

bool MementoUndoTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const int n = args.value("n", 1);
	if (!ctx.sceneMgr->undo(n)) {
		return ctx.result(id, "Failed to undo", false);
	}
	return ctx.result(id, "Undo successful", false);
}

} // namespace voxedit
