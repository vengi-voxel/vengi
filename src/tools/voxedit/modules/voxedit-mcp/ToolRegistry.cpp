/**
 * @file
 */

#include "ToolRegistry.h"
#include "core/collection/DynamicArray.h"
#include "core/Log.h"

namespace voxedit {

ToolRegistry::~ToolRegistry() {
	shutdown();
}

void ToolRegistry::shutdown() {
	for (const auto &pair : _tools) {
		if (pair && pair->second) {
			delete pair->second;
		}
	}
	_tools.clear();
}

void ToolRegistry::registerTool(Tool *tool) {
	_tools.put(tool->name(), tool);
}

bool ToolRegistry::unregisterTool(const core::String &toolName) {
	const auto it = _tools.find(toolName);
	if (it != _tools.end()) {
		if (it->second) {
			delete it->second;
		}
		_tools.erase(it);
		return true;
	}
	return false;
}

bool ToolRegistry::call(const core::String &toolName, const json::Json &id, const json::Json &input,
						ToolContext &ctx) {
	const auto it = _tools.find(toolName);
	if (it != _tools.end()) {
		Tool *tool = it->second;
		return tool->execute(id, input, ctx);
	}
	Log::warn("Tool '%s' not found", toolName.c_str());
	return false;
}

void ToolRegistry::addRegisteredTools(json::Json &tools) {
	core::DynamicArray<Tool *> sorted;
	sorted.reserve(_tools.size());
	for (const auto &pair : _tools) {
		sorted.push_back(pair->second);
	}
	sorted.sort([](const Tool *lhs, const Tool *rhs) { return lhs->name() > rhs->name(); });
	for (Tool *tool : sorted) {
		tools.push(tool->inputSchema());
	}
}

} // namespace voxedit