/**
 * @file
 */

#pragma once

#include "commonlua/LUA.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "noise/Noise.h"
#include "voxel/Face.h"
#include "voxelgenerator/LUAApi.h"
#include <glm/vec3.hpp>

namespace scenegraph {
class SceneGraph;
class SceneGraphNode;
} // namespace scenegraph

namespace voxel {
class Region;
} // namespace voxel

namespace voxedit {

struct BrushContext;
class ModifierVolumeWrapper;

/**
 * @brief A selection mode driven by a user-supplied Lua script
 *
 * Each LUASelectionMode loads a single Lua script that defines selection behavior:
 * - @c select(node, region, ...) - perform the selection (required)
 * - @c arguments() - declare parameters for the UI (optional)
 * - @c description() - return a human-readable description (optional)
 * - @c icon() - return a Lucide icon name (optional)
 *
 * Scripts access the selection context through the @c g_selectioncontext global,
 * which provides cursor position, face, modifier type, and first-click position.
 *
 * Selection is performed by calling @c volume:setSelected(x, y, z, true/false) on
 * the node's volume.
 *
 * @ingroup Brushes
 * @sa LuaBrush
 */
class LUASelectionMode {
private:
	mutable lua::LUA _lua;
	noise::Noise _noise;
	voxelgenerator::LuaDirtyRegions _dirtyRegions;
	io::FilesystemPtr _filesystem;

	core::String _scriptFilename;
	core::String _scriptSource;
	core::String _description;
	core::String _iconName;
	bool _scriptLoaded = false;

	core::DynamicArray<voxelgenerator::LUAParameterDescription> _parameterDescription;
	core::DynamicArray<core::String> _parameters;

	bool initLuaState();

public:
	explicit LUASelectionMode(const io::FilesystemPtr &filesystem);
	~LUASelectionMode() = default;

	bool init();
	void shutdown();

	/**
	 * @brief Load a selection mode script from the given filename
	 * @param filename Relative path within the filesystem (e.g. "slope")
	 * @return true if the script was loaded and has a select() function
	 */
	bool loadScript(const core::String &filename);

	/**
	 * @brief Execute the selection mode script
	 *
	 * Called from SelectBrush::generate() when this lua mode is active.
	 * The script's select() function receives: node, region, plus any user arguments.
	 * @param aabbFirstPos The position of the first click (from AABBBrush)
	 * @param aabbFace The face from the initial click (from AABBBrush)
	 */
	void execute(scenegraph::SceneGraph &sceneGraph, ModifierVolumeWrapper &wrapper, const BrushContext &ctx,
				 const voxel::Region &region, const glm::ivec3 &aabbFirstPos, voxel::FaceNames aabbFace);

	/**
	 * @brief Get the short name of this selection mode for display
	 */
	core::String scriptName() const;

	const core::String &scriptDescription() const;

	const char *iconString() const;

	const core::DynamicArray<voxelgenerator::LUAParameterDescription> &parameterDescriptions() const;

	core::DynamicArray<core::String> &parameters();
	const core::DynamicArray<core::String> &parameters() const;

	bool isLoaded() const;

	/**
	 * @brief Generate a JSON representation of the selection mode Lua API to a stream
	 */
	bool apiJsonToStream(io::WriteStream &stream);
};

inline const core::String &LUASelectionMode::scriptDescription() const {
	return _description;
}

inline const core::DynamicArray<voxelgenerator::LUAParameterDescription> &
LUASelectionMode::parameterDescriptions() const {
	return _parameterDescription;
}

inline core::DynamicArray<core::String> &LUASelectionMode::parameters() {
	return _parameters;
}

inline const core::DynamicArray<core::String> &LUASelectionMode::parameters() const {
	return _parameters;
}

inline bool LUASelectionMode::isLoaded() const {
	return _scriptLoaded;
}

} // namespace voxedit
