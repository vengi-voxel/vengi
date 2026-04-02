/**
 * @file
 */

#pragma once

#include "AABBBrush.h"
#include "commonlua/LUA.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "noise/Noise.h"
#include "voxelgenerator/LUAApi.h"

namespace voxedit {

/**
 * @brief A brush driven by a user-supplied Lua script
 *
 * Each LuaBrush loads a single Lua script that defines brush behavior via callbacks:
 * - @c generate(node, region, color, ...) - place voxels (required)
 * - @c calcregion(cx, cy, cz, ...) - return 6 ints (minX,minY,minZ,maxX,maxY,maxZ) for preview region (optional)
 * - @c arguments() - declare parameters for the UI (optional)
 * - @c description() - return a human-readable description (optional)
 *
 * Multiple LuaBrush instances can coexist, each with its own Lua state and parameters.
 *
 * @ingroup Brushes
 * @sa AABBBrush
 */
class LuaBrush : public AABBBrush {
private:
	using Super = AABBBrush;

	mutable lua::LUA _lua;
	noise::Noise _noise;
	voxelgenerator::LuaDirtyRegions _dirtyRegions;
	io::FilesystemPtr _filesystem;

	core::String _scriptFilename;
	core::String _scriptSource;
	core::String _description;
	core::String _iconName;
	bool _scriptLoaded = false;
	bool _hasCalcRegion = false;
	bool _hasGizmo = false;
	bool _hasApplyGizmo = false;
	bool _useSimplePreview = false;
	bool _wantAABB = false;

	core::DynamicArray<voxelgenerator::LUAParameterDescription> _parameterDescription;
	core::DynamicArray<core::String> _parameters;

	bool initLuaState();
	static uint32_t mapGizmoOperation(const char *name);

	bool wantAABB() const override;

protected:
	void generate(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				  const voxel::Region &region) override;

public:
	LuaBrush(const io::FilesystemPtr &filesystem);

	/**
	 * @brief Load a brush script from the given filename
	 * @param filename Relative path within the filesystem (e.g. "brushes/sphere.lua")
	 * @return true if the script was loaded and has a generate() function
	 */
	bool loadScript(const core::String &filename);

	/**
	 * @brief Get the short name of this brush for display
	 */
	core::String scriptName() const;

	/**
	 * @brief Get the description from the script's description() callback
	 */
	const core::String &scriptDescription() const;

	/**
	 * @brief Get the parameter descriptions for the UI
	 */
	const core::DynamicArray<voxelgenerator::LUAParameterDescription> &parameterDescriptions() const;

	/**
	 * @brief Get/set the current parameter values
	 */
	core::DynamicArray<core::String> &parameters();
	const core::DynamicArray<core::String> &parameters() const;

	voxel::Region calcRegion(const BrushContext &ctx) const override;
	void update(const BrushContext &ctx, double nowSeconds) override;
	bool active() const override;
	void reset() override;
	bool init() override;
	void shutdown() override;

	bool wantBrushGizmo(const BrushContext &ctx) const override;
	void brushGizmoState(const BrushContext &ctx, BrushGizmoState &state) const override;
	bool applyBrushGizmo(BrushContext &ctx, const glm::mat4 &matrix, const glm::mat4 &deltaMatrix,
						 uint32_t operation) override;

	/**
	 * @brief Whether the script requested simple (outline-only) preview mode
	 */
	bool useSimplePreview() const;

	/**
	 * @brief Get the icon string for this script brush (mapped from the Lua icon() callback)
	 * @return A Lucide icon string (ICON_LC_*), or ICON_LC_SCROLL as the default
	 */
	const char *iconString() const;

	/**
	 * @brief Generate a JSON representation of the brush Lua API to a stream
	 */
	bool apiJsonToStream(io::WriteStream &stream);
};

inline const core::String &LuaBrush::scriptDescription() const {
	return _description;
}

inline const core::DynamicArray<voxelgenerator::LUAParameterDescription> &LuaBrush::parameterDescriptions() const {
	return _parameterDescription;
}

inline core::DynamicArray<core::String> &LuaBrush::parameters() {
	return _parameters;
}

inline const core::DynamicArray<core::String> &LuaBrush::parameters() const {
	return _parameters;
}

inline bool LuaBrush::useSimplePreview() const {
	return _useSimplePreview;
}

} // namespace voxedit
