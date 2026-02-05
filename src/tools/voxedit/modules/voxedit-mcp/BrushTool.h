/**
 * @file
 */

#pragma once

#include "Tool.h"
#include "voxedit-util/modifier/ModifierType.h"
#include "voxedit-util/modifier/brush/BrushType.h"

namespace voxedit {

/**
 * @brief Base class for brush tools that use the Modifier to manipulate voxels
 *
 * This provides common functionality for all brush-based MCP tools including:
 * - Setting up brush parameters (cursor position, modifier type, AABB mode)
 * - Executing the brush operation on the target node
 * - Common parameter schema generation
 */
class BrushTool : public Tool {
protected:
	/**
	 * @brief Parse modifier type from string argument
	 * @return ModifierType corresponding to the string, or Place as default
	 */
	static ModifierType parseModifierType(const core::String &type);

	/**
	 * @brief Parse brush mode from string argument
	 * @return Brush mode flag (AABB, single, center)
	 */
	static uint32_t parseBrushMode(const core::String &mode);

	/**
	 * @brief Common JSON properties for modifier type selection
	 */
	static nlohmann::json propModifierType();

	/**
	 * @brief Common JSON properties for brush mode selection
	 */
	static nlohmann::json propBrushMode();

	/**
	 * @brief Common JSON properties for AABB region (mins and maxs)
	 */
	static nlohmann::json propRegion();

	/**
	 * @brief Common JSON property for color index
	 */
	static nlohmann::json propColorIndex();

	/**
	 * @brief Common JSON property for position
	 */
	static nlohmann::json propPosition(const core::String &description);

	/**
	 * @brief Execute a brush operation with the given parameters
	 *
	 * @param ctx Tool context with scene manager
	 * @param nodeUUID Target node UUID
	 * @param brushType The brush type to use
	 * @param modifierType The modifier operation (Place, Erase, Override, Paint)
	 * @param colorIndex Palette color index for the voxel
	 * @param mins Minimum corner of the AABB region
	 * @param maxs Maximum corner of the AABB region
	 * @param id JSON-RPC request id
	 * @return true if the operation succeeded
	 */
	bool executeBrush(ToolContext &ctx, const core::UUID &nodeUUID, BrushType brushType, ModifierType modifierType,
					  int colorIndex, const glm::ivec3 &mins, const glm::ivec3 &maxs, const nlohmann::json &id);

public:
	BrushTool(const core::String &name);
	virtual ~BrushTool() = default;
};

} // namespace voxedit
