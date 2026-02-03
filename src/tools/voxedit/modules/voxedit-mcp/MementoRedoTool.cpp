/**
 * @file
 */

#include "MementoRedoTool.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

MementoRedoTool::MementoRedoTool() : Tool("voxedit_memento_redo") {
	_tool["description"] = "Redo the last undone action (optional n argument)";

	nlohmann::json nprop = propTypeDescription("integer", "Number of redo steps");
	nprop["default"] = 1;

	nlohmann::json in;
	in["type"] = "object";
	in["properties"]["n"] = core::move(nprop);
	_tool["inputSchema"] = core::move(in);
}

bool MementoRedoTool::execute(const nlohmann::json &id, const nlohmann::json &args, ToolContext &ctx) {
	const int n = args.value("n", 1);
	if (!ctx.sceneMgr->redo(n)) {
		return ctx.result(id, "Failed to redo", false);
	}
	return ctx.result(id, "Redo successful", false);
}

} // namespace voxedit
