/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "MeshMaterial.h"
#include "util/Version.h"
#include "voxel/Face.h"

namespace voxel {
class RawVolumeWrapper;
}

namespace voxelformat {

/**
 * @brief Blockbench bbmodel json format
 *
 * In blockbench a scene is built by elements (volumes) and nodes in the outliner (groups).
 * The elements are referenced by the nodes.
 * A cube is made by 6 faces with uv coordinates and a texture index. A voxel (1x1x1 cube in blockbench) can therefore
 * have 6 different colors. This is not possible in vengi. We therefore define an order of faces where one overwrites
 * the other. This order is hardcoded from negative to positive from x to y to z. This basically means that the front
 * and the back sides are the dominant ones - followed by the up and down sides - and finally the left and right sides.
 *
 * @section bbmodel_format Blockbench BBModel Format Documentation
 *
 * @subsection bbmodel_overview Overview
 * The bbmodel format is a JSON-based file format used by Blockbench (https://www.blockbench.net/) for 3D modeling.
 * It supports hierarchical scene structures with elements (cubes/meshes), groups (bones), animations, and textures.
 *
 * @subsection bbmodel_structure File Structure
 *
 * @par Meta Section
 * @li @c format_version: Version string (e.g., "4.5", "4.10", "5.0")
 * @li @c model_format: Format type ("free", "bedrock", "java_block", etc.). Only "free" is fully supported.
 * @li @c box_uv: Boolean indicating if box UV mapping is used
 * @li @c creation_time: Unix timestamp (optional)
 *
 * @par Resolution
 * @li @c width: Texture width in pixels (default: 16)
 * @li @c height: Texture height in pixels (default: 16)
 *
 * @par Elements Array
 * Contains the actual geometry (cubes or meshes). Each element has:
 * @li @c uuid: Unique identifier (referenced by outliner)
 * @li @c name: Display name
 * @li @c type: "cube" or "mesh"
 * @li @c origin: Pivot point in world coordinates [x, y, z] (used for rotation)
 * @li @c rotation: Rotation in degrees [x, y, z]
 * @li @c from / @c to: Cube bounds in world coordinates (for type="cube")
 * @li @c faces: Object with face data (north, east, south, west, up, down)
 *   - Each face has @c uv array [x1, y1, x2, y2] and @c texture index
 * @li @c vertices / @c faces: Mesh data (for type="mesh")
 * @li @c locked: Boolean indicating if element is locked
 * @li @c box_uv: Per-element UV mapping mode
 * @li @c color: Color index (0-15, where -1 is none)
 *
 * @par Outliner Array
 * Defines the scene hierarchy. Can contain:
 * @li @b String @b UUIDs: Direct references to elements
 * @li @b Group @b Objects: Hierarchical nodes (bones) with:
 * @li @c uuid: Unique identifier
 * @li @c name: Display name
 * @li @c origin: Pivot point in world coordinates [x, y, z]
 * @li @c rotation: Rotation in degrees [x, y, z]
 * @li @c children: Array of UUIDs or nested groups
 * @li @c visibility: Boolean for visibility
 * @li @c locked: Boolean for locked state
 * @li @c mirror_uv: Boolean for UV mirroring
 * @li @c color: Color index
 *
 * @par Textures Array
 * @li @c uuid: Unique identifier
 * @li @c name: Display name
 * @li @c path: Absolute file path (optional)
 * @li @c relative_path: Path relative to model file (optional)
 * @li @c source: Base64 encoded image data (data:image/png;base64,...)
 * @li @c width / @c height: Image dimensions
 * @li @c uv_width / @c uv_height: UV space dimensions (for per-texture UV sizing)
 *
 * @par Animations Array (optional)
 * @li @c uuid: Unique identifier
 * @li @c name: Animation name
 * @li @c loop: "once", "loop", or "hold"
 * @li @c length: Duration in seconds
 * @li @c snapping: Frame snapping value
 * @li @c animators: Object mapping node UUIDs to animator data. Each animator has:
 * @li - @c name: Node name
 * @li - @c type: "bone" or "cube"
 * @li - @c keyframes: Array of keyframe objects
 * @li -- @c channel: "position", "rotation", or "scale"
 * @li -- @c time: Time in seconds
 * @li -- @c data_points: Array of vec3 values (can be objects with x/y/z or arrays)
 * @li -- @c interpolation: "linear", "catmullrom", "bezier", "ease_in", "ease_out", "ease_in_out"
 * @li -- @c bezier_left_time / @c bezier_left_value: Left bezier control point
 * @li -- @c bezier_right_time / @c bezier_right_value: Right bezier control point
 * @li -- @c bezier_linked: Boolean if handles are linked
 *
 * @subsection bbmodel_coordinates Coordinate System
 *
 * Blockbench uses a @b right-handed @b Y-up coordinate system:
 * @li X: Right (positive) / Left (negative)
 * @li Y: Up (positive) / Down (negative)
 * @li Z: Forward/South (positive) / Backward/North (negative)
 *
 * Face naming relative to a cube:
 * @li north: -Z face (front)
 * @li south: +Z face (back)
 * @li east: +X face (right)
 * @li west: -X face (left)
 * @li up: +Y face (top)
 * @li down: -Y face (bottom)
 *
 * @subsection bbmodel_transforms Transform Hierarchy
 *
 * @par Element Transforms
 * Elements have transform-related properties:
 * @li @b origin: The pivot point for rotation (in world coordinates)
 * @li @b rotation: Euler angles in degrees [x, y, z]
 * @li @b from / @b to: The cube bounds (in world coordinates)
 *
 * @note @b IMPORTANT: Element positions (from/to) are stored in world/absolute coordinates in the bbmodel file,
 * but when building a scene graph hierarchy, they must be converted to parent-relative coordinates.
 * This is done by subtracting the parent group's origin from the element's position.
 *
 * @par Group (Bone) Transforms
 * Groups define transformation hierarchies:
 * @li @b origin: The pivot/joint position (in world coordinates for root groups, parent-relative for nested groups)
 * @li @b rotation: Euler angles in degrees [x, y, z]
 * @li Children inherit parent transformations
 *
 * @par Transform Application Order
 * For proper transform application:
 * @li Groups establish a bone hierarchy with their @c origin as pivot points
 * @li Each group's @c origin is its local position (relative to parent)
 * @li Elements referenced by a group must have their world positions converted to parent-relative:
 *    @code relativePos = elementWorldPos - parentGroupOrigin @endcode
 * @li Elements have their own @c origin for local rotation pivot
 * @li World transform = Parent * LocalTranslation * LocalRotation * LocalScale
 *
 * @subsection bbmodel_implementation Implementation Notes
 *
 * @par Pivot Handling
 * In vengi, pivots are normalized to [0,1] relative to the volume region.
 * In Blockbench, @c origin is in world coordinates.
 * @code pivot = (origin - cube.from) / (cube.to - cube.from) @endcode
 *
 * @par Face Priority
 * When a cube has 6 faces with potentially different textures/colors:
 * Priority order (first overwrites later): front/back (Z) > up/down (Y) > left/right (X)
 *
 * @par Version Compatibility
 * @li Pre-3.2: Z-axis rotation was inverted (handled in processCompatibility)
 * @li Pre-4.10: Texture paths had different handling
 * @li Pre-5.0: Animation data_points used inverted molang expressions for X/Y
 *
 * @sa https://www.blockbench.net/wiki/docs/bbmodel
 * @sa https://github.com/JannisX11/blockbench/blob/master/js/io/formats/bbmodel.js
 *
 * @ingroup Formats
 */
class BlockbenchFormat : public MeshFormat {
public:
	struct BBCubeFace {
		glm::vec2 uvs[2]{{0.0f, 1.0f}, {0.0f, 1.0f}};
		int textureIndex = -1;
	};

	struct BBCube {
		BBCubeFace faces[(int)voxel::FaceNames::Max];
		glm::vec3 from{0.0f};
		glm::vec3 to{0.0f};
	};

	struct BBNode {
		core::UUID uuid;
		core::String name;
		// in degrees
		glm::vec3 rotation{0.0f};
		// in world coordinates
		glm::vec3 origin{0.0f};
		glm::vec3 size{1.0f};
		bool locked = false;
		bool visible = true;
		bool mirror_uv = false;
		int color = 0;
		// group nodes
		core::DynamicArray<BBNode> children;
		// elements (volumes) by uuid
		core::DynamicArray<core::UUID> referenced;
	};

	enum class BBElementType { Cube, Mesh, Max };

	struct BBElement {
		core::UUID uuid;
		core::String name;
		// in degrees
		glm::vec3 rotation{0.0f};
		// in world coordinates
		glm::vec3 origin{0.0f}; // pivot - not normalized
		bool rescale = false;
		bool locked = false;
		bool box_uv = false;
		int color = 0;
		BBCube cube;
		Mesh mesh;
		BBElementType type = BBElementType::Max;
	};

	struct BBMeta {
		// 1654934558
		uint64_t creationTimestamp = 0;
		bool box_uv = false;
		util::Version version{0, 0};
		// model_format: free bedrock bedrock_old java_block animated_entity_model skin
		core::String modelFormat;
		core::String formatVersion;
		core::String name;
		core::String model_identifier;
		glm::ivec2 resolution;
	};

	// map via uuid
	using BBElementMap = core::Map<core::UUID, BBElement, 11, core::UUIDHash>;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Blockbench", {"bbmodel"}, {}, VOX_FORMAT_FLAG_MESH};
		return f;
	}

private:
	bool addNode(const BBNode &node, const BBElementMap &elementMap, scenegraph::SceneGraph &sceneGraph,
				 const MeshMaterialArray &meshMaterialArray, int parent) const;
	bool generateCube(const BBNode &node, const BBElement &element, const MeshMaterialArray &meshMaterialArray,
					  scenegraph::SceneGraph &sceneGraph, int parent) const;
	bool generateMesh(const BBNode &node, BBElement &element, const MeshMaterialArray &meshMaterialArray,
					  scenegraph::SceneGraph &sceneGraph, int parent) const;

protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override {
		return false;
	}
};

} // namespace voxelformat
