/**
 * @file
 */

#pragma once

#include "Tool.h"
#include "core/collection/DynamicStringMap.h"

namespace voxedit {

class ToolRegistry {
private:
	core::DynamicStringMap<Tool *> _tools;

public:
	ToolRegistry() = default;
	~ToolRegistry();

	void shutdown();
	void registerTool(Tool *tool);
	bool unregisterTool(const core::String &toolName);
	/**
	 * @return @c true if the call was succesfully executed - @c false otherwise
	 */
	bool call(const core::String &toolName, const json::Json &id, const json::Json &input, ToolContext &ctx);
	void addRegisteredTools(json::Json &tools);

	inline const core::DynamicStringMap<Tool *> &tools() const {
		return _tools;
	}
};

} // namespace voxedit