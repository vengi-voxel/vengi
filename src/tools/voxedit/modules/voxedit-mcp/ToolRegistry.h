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

protected:
	const core::String _name;
	nlohmann::json _tool;

public:
	ToolRegistry() = default;
	~ToolRegistry();

	void shutdown();
	void registerTool(Tool *tool);
	bool unregisterTool(const core::String &toolName);
	/**
	 * @return @c true if the call was succesfully executed - @c false otherwise
	 */
	bool call(const core::String &toolName, const nlohmann::json &id, const nlohmann::json &input, ToolContext &ctx);
	void addRegisteredTools(nlohmann::json &tools);

	inline const core::DynamicStringMap<Tool *> &tools() const {
		return _tools;
	}
};

} // namespace voxedit