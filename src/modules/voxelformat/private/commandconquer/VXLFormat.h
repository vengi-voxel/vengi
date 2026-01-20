/**
 * @file
 */

#pragma once

#include "VXLShared.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Tiberian Sun Voxel Animation Format
 *
 * @li http://xhp.xwis.net/documents/VXL_Format.txt
 * @li https://modenc.renegadeprojects.com/Voxel#Rendering
 *
 * The format uses z-up as coordinate system, x to the right, y towards the viewer
 *
 * @section vxl_format Format Structure
 *
 * The VXL format stores voxel data as a collection of sections, where each section represents an
 * independent component of the model (such as body, turret, barrel, or animated parts like legs or rotors).
 * Individual voxels (volumetric pixels) are encoded as five-value tuples containing:
 * - X, Y, Z position coordinates within the section
 * - Color index referencing the palette
 * - Normal index for lighting calculations
 *
 * The actual color rendered for each voxel depends on the loaded palette, while the surface orientation
 * indicated by the normal index depends on the normals generation algorithm and the normals lookup table.
 *
 * @subsection vxl_format_limits Format Limitations
 *
 * The VXL format enforces a maximum dimension of 255 units per axis, resulting in a maximum bounding
 * volume of 255×255×255 voxels. Models approaching these limits often exhibit graphical artifacts including
 * rendering glitches and palette errors. This constraint can be problematic for large-scale models such as
 * capital ships or long aircraft, though most practical voxel models remain well under 100 units on the
 * longest axis.
 *
 * @subsection vxl_format_hva HVA File Dependency
 *
 * Every VXL file requires a corresponding HVA (Hierarchical Voxel Animation) file that defines the spatial
 * positioning of each section relative to the world origin (0,0,0) on the X, Y, and Z axes. The HVA file
 * also controls turret and barrel alignment, rotation pivot points, and animation sequences for multi-section
 * models. Missing HVA files cause immediate engine errors and prevent the model from loading.
 *
 * @subsection vxl_format_multisection Multi-Section Models
 *
 * VXL files can contain multiple hierarchical sections within a single file, creating complex articulated
 * models. Notable examples include:
 *
 * - **Mammoth Mk. II (Tiberian Sun)**: 13 sections comprising the body, four upper legs, four lower legs,
 *   four feet, and turret, paired with a complex HVA to produce walking animations
 * - **Helicopters (Red Alert 2/Yuri's Revenge)**: Separate rotor sections animated independently from the
 *   body, simplifying rotation effects
 *
 * Multi-section models enable sophisticated animations while keeping related components in a single file.
 * The modding community has extensively utilized this capability for custom animated units.
 *
 * @section vxl_rendering Rendering Characteristics
 *
 * @subsection vxl_rendering_scale Scale and Proportions
 *
 * When a voxel model is oriented parallel to the camera (front-facing), the voxel-to-pixel ratio is exactly
 * 1:1. A model measuring 60 voxels in width appears as 60 pixels on screen when viewed directly from the side.
 *
 * When rotated to isometric viewing angles, voxel dimensions transform to fit the game's projection:
 * - **Tiberian Sun**: A flat square model of 33.94 voxels per side fits precisely within one game cell
 * - **Red Alert 2/Yuri's Revenge**: A flat square model of 42.43 voxels per side fits precisely within one cell
 *
 * One voxel unit equals approximately 6.034 leptons (the game's internal distance unit), consistent across
 * both Tiberian Sun and Red Alert 2 since leptons are defined relative to cell size.
 *
 * While individual voxel size is fixed, the spacing between voxels can be adjusted using bounds manipulation.
 * Modifying the bounds effectively scales the entire model by expanding or contracting the space between
 * voxels, creating larger or smaller rendered models without altering the voxel data itself.
 *
 * @subsection vxl_rendering_rotation Dynamic Rotation
 *
 * The game engine supports 32,768 unique rotation angles (32 facings per axis) for each voxel model.
 * To optimize performance and memory usage, rotated views are rendered on-demand rather than pre-computed.
 * Once a specific rotation angle is rendered, it is cached for the duration of the game session, balancing
 * visual fidelity with hardware constraints—a critical optimization for the era when these games were developed.
 *
 * @subsection vxl_rendering_layering Draw Order and Layering
 *
 * For multi-section voxel models, the rendering order follows the section index sequence. Section 0 renders
 * at the bottom layer, section 1 renders above it, and subsequent sections stack progressively higher. This
 * ordering is evident in models like the Mammoth Mk. II (HMEC.VXL) and its corresponding HVA file.
 *
 * For units with turrets and barrels:
 * - The turret renders above the body (similar to traditional 2D tank sprites)
 * - The barrel renders above the turret, but only when the turret faces toward the camera
 * - This view-dependent rendering prevents the barrel from incorrectly obscuring parts of the model when
 *   the turret points away from the viewer
 *
 * @section vxl_bounds Voxel Bounds and Coordinate System
 *
 * Voxel bounds specify the dimensions and position of a voxel container within the game world, determining
 * its center of rotation. When a unit is positioned in a cell, the game treats the center of that cell's
 * ground plane as the origin point (0,0,0) for the voxel container. This origin is shared by all components
 * of a unit (body, turret, and barrel), ensuring they rotate around common axes. The vertical Z-axis is
 * especially critical as it serves as the primary rotation axis when units change direction.
 *
 * @subsection vxl_bounds_tech Bounds Representation
 *
 * Bounds are defined using six integer values: MinX, MinY, MinZ, MaxX, MaxY, MaxZ. These represent the
 * coordinates of two opposite corners of the bounding box relative to the origin. The minimum values
 * correspond to the back-lower-left corner, while the maximum values represent the front-upper-right corner.
 *
 * The dimensions of a voxel container are calculated as:
 * - Width: (MaxX - MinX)
 * - Depth: (MaxY - MinY)
 * - Height: (MaxZ - MinZ)
 *
 * The rotational center always remains at the origin (0,0,0).
 *
 * Following this system, a turret's bounding box should be positioned atop the unit body, and the barrel's
 * bounding box should extend forward from the turret. The voxel geometry will be automatically scaled to
 * fit within its bounds if necessary, allowing for dimension adjustments without redrawing. For instance,
 * halving the min/max X values while doubling the Y values will create a unit that is half as long and
 * twice as wide.
 *
 * @note For train voxels, set MinZ to at least 3 to prevent track graphics from overlapping the train model.
 *
 * @subsection vxl_bounds_example Example: Rhino Tank
 *
 * The following bounds demonstrate proper component positioning for the Rhino Tank:
 *
 * Body (htnk.vxl):
 * - Min: (-19, -17, 0)
 * - Max: (26, 16.5, 11.5)
 * - Dimensions: 45 × 33.5 × 11.5
 *
 * Turret (htnktur.vxl):
 * - Min: (-16, -9, 11.5)
 * - Max: (11.5, 9, 20)
 * - Dimensions: 28.5 × 18 × 8.5
 *
 * Barrel (htnkbarl.vxl):
 * - Min: (12, -1.5, 13)
 * - Max: (30.5, 1.5, 16)
 * - Dimensions: 17.5 × 3 × 29
 *
 * Key observations:
 * - The body is positioned at ground level (MinZ = 0)
 * - The turret sits directly atop the body (turret MinZ ≈ body MaxZ)
 * - The barrel extends forward from the turret (barrel MinX ≈ turret MaxX)
 * - The rotation axes are offset toward the rear due to asymmetric Min/Max X values
 *
 * @subsection vxl_bounds_scaling Scaling and Visual Optimization
 *
 * Bounds can be used to scale voxels along any axis, providing fine-grained size control without
 * sacrificing detail. This technique is particularly effective for addressing rendering artifacts such
 * as "black dot syndrome," which occurs when single-layer voxel surfaces render with visible gaps at
 * certain angles. These gaps expose underlying voxels with inverted normals, causing incorrect lighting
 * and appearing as dark spots.
 *
 * Scaling the bounds to 90% (multiplying all values by 0.9) typically eliminates these gaps by tightening
 * the voxel geometry. While adding a second layer with matching normal values (as automated by VXLSE's
 * auto-normals tool) is the optimal solution, bounds scaling offers a simpler alternative with additional
 * benefits.
 *
 * @warning The VXL renderer blends colors of overlapping voxels. When scaling is applied uniformly,
 * this blending affects all directions and angles. Small remappable (faction-colored) regions may blend
 * with adjacent colors, potentially altering the perceived house color. For example, blue units with red
 * faction colors may blend to purple, causing visual confusion. To mitigate this, surround remappable
 * areas with grayscale textures, ensuring color blending only affects brightness rather than hue.
 *
 * Always test voxel scaling effects in-game, paying careful attention to color rendering and texture
 * behavior across different viewing angles.
 *
 * @section vxl_turret_positioning Turret and Barrel Positioning
 *
 * Proper positioning of turrets and barrels in multi-section voxel units is achieved through one of
 * three methods, listed here in order of correctness and recommended practice:
 *
 * @subsection vxl_positioning_header VXL Header Bounds (Recommended)
 *
 * The correct and semantically proper method is to define component positions through the bounds values
 * in the VXL file header. This approach maintains the separation of concerns where the VXL format handles
 * spatial positioning and the HVA format handles temporal animation. Modern tools such as OS HVA Builder
 * correctly edit the VXL header bounds when adjusting turret and barrel positions, making this the
 * recommended workflow.
 *
 * By modifying the MinX, MinY, MinZ, MaxX, MaxY, and MaxZ values in each component's header, you directly
 * control where turrets sit relative to the body and where barrels extend from the turret. This method
 * produces voxel models that work with any generic HVA file appropriate for the unit type, improving
 * modularity and maintainability.
 *
 * @subsection vxl_positioning_ini INI Configuration Adjustments
 *
 * Additional fine-tuning can be achieved through the TurretOffset tag in the unit's art(md).ini definition.
 * This tag provides runtime positioning adjustments without modifying the voxel files themselves, allowing
 * for quick tweaks during game balance or visual polish phases. However, this should be used for minor
 * adjustments rather than primary positioning, as the base position should still be defined correctly in
 * the VXL headers.
 *
 * @subsection vxl_positioning_hva HVA Modification (Deprecated)
 *
 * Historically, modders used HVA editors to position turrets and barrels by modifying the transformation
 * matrices in the HVA file. While functional, this approach is semantically incorrect and not recommended.
 * The HVA format (Hierarchical Voxel Animation) is intended exclusively for defining animation sequences
 * over time, not for static spatial positioning.
 *
 * Using HVA files for positioning creates several problems:
 * - Each voxel model requires a unique custom HVA file, even for non-animated units
 * - Violates the separation between spatial structure (VXL) and temporal animation (HVA)
 * - Reduces reusability of animation files across similar units
 * - Complicates asset management and version control
 *
 * Modern workflows should avoid this method entirely in favor of proper VXL header editing.
 *
 * @see HVAFormat for information about the animation format
 *
 * @ingroup Formats
 */
class VXLFormat : public PaletteFormat {
private:
	glm::ivec3 maxSize() const override {
		return glm::ivec3(255);
	}

	// writing
	bool writeLayerBodyEntry(io::SeekableWriteStream &stream, const voxel::RawVolume *volume, int x, int y,
							 int z, uint8_t skipCount, uint8_t voxelCoun) const;
	bool writeLayer(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
					const scenegraph::SceneGraphNode &node, vxl::VXLLayerOffset &offsets,
					uint64_t nodeSectionOffset) const;
	bool writeLayerHeader(io::SeekableWriteStream &stream, const scenegraph::SceneGraphNode &node,
						  uint32_t nodeIdx) const;
	bool writeLayerInfo(io::SeekableWriteStream &stream, const scenegraph::SceneGraph &sceneGraph,
						const scenegraph::SceneGraphNode &node, const vxl::VXLLayerOffset &offsets) const;
	bool writeHeader(io::SeekableWriteStream &stream, uint32_t numNodes, const palette::Palette &palette);

	// reading
	bool readLayerHeader(io::SeekableReadStream &stream, vxl::VXLModel &mdl, uint32_t nodeIdx) const;
	bool readLayerInfo(io::SeekableReadStream &stream, vxl::VXLModel &mdl, uint32_t nodeIdx) const;
	bool readLayer(io::SeekableReadStream &stream, vxl::VXLModel &mdl, uint32_t nodeIdx,
				   scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette) const;
	bool readLayers(io::SeekableReadStream &stream, vxl::VXLModel &mdl, scenegraph::SceneGraph &sceneGraph,
					const palette::Palette &palette) const;
	bool readLayerInfos(io::SeekableReadStream &stream, vxl::VXLModel &mdl) const;
	bool readLayerHeaders(io::SeekableReadStream &stream, vxl::VXLModel &mdl) const;

	bool saveVXL(const scenegraph::SceneGraph &sceneGraph,
				 core::Buffer<const scenegraph::SceneGraphNode *> &nodes, const core::String &filename,
				 const io::ArchivePtr &archive);

	bool prepareModel(vxl::VXLModel &mdl) const;
	bool readHeader(io::SeekableReadStream &stream, vxl::VXLModel &mdl, palette::Palette &palette);

protected:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;

	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;
					public:

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Tiberian Sun",
									"",
									{"vxl"},
									{"Voxel Animation"},
									VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_ANIMATION | FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
