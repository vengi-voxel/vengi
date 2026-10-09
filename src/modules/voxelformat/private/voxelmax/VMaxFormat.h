/**
 * @file
 */

#pragma once

#include "core/UUID.h"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include "voxelformat/Format.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "util/BinaryPList.h"

namespace voxelformat {

/**
 * @brief VoxelMax (*.vmax, *.vmax.zip, *.vmaxb)
 *
 * NSFileWrapper package: a zip (`*.vmax.zip` / `*.vmax`) or an unpacked directory (`Name.vmax/`)
 * with `scene.json` plus `contents*.vmaxb` and `palette*.png`. Opening a `contents.vmaxb` that
 * sits next to `scene.json` loads the whole package, not that single leftover object.
 *
 * up to 2040 materials
 * up to 512x512x512 working area
 * unlimited history
 * z points upwards. scene.json t_r is SceneKit axis-angle (xyz + radians), not a quaternion.
 * Volume voxels and node TRS remap (x, y, z_up) to vengi (x, z_up, y).
 * Transforms stay on nodes; volumes are cropped and the origin shift is folded into TRS.
 *
 * ------------------
 *
 * scene.json gives the scene nodes hierarchy, groups and volumes.
 * palette.png holds colors. palette.settings.vmaxpsb (and older vmaxb pal dict)
 * stores 8 layer materials (mi/sic/mc/rc/tc/md/sh) plus optional MagicaVoxel voxmats.
 * each material(max 8) of an object is rendered as a separate sub-mesh and the position of those are offset by the
 * pivot given by the combination e_c and , t_al, t_pf, t_pa, t_po
 *
 * Group Info
 * * case uuid = "id"
 * * case index = "ind"
 * * case parent = "pid"
 * * case selected = "s"
 * * case hidden = "h"
 * * case position = "t_p"
 * * case rotation = "t_r"
 * * case scale = "t_s"
 * * case alignment = "t_al"
 * * case pivotFace = "t_pf"
 * * case pivotAlign = "t_pa"
 * * case pivotOffset = "t_po"
 * * case center = "e_c"
 * * case boundsMin = "e_mi"
 * * case boundsMax = "e_ma"
 * * case name = "name"
 *
 * Object Info
 * * case uuid = "id"
 * * case parent = "pid"
 * * case index = "ind"
 * * case selected = "s"
 * * case hidden = "h"
 * * case contents = "data"
 * * case history = "hist"
 * * case palette = "pal"
 * * case position = "t_p"
 * * case rotation = "t_r"
 * * case scale = "t_s"
 * * case alignment = "t_al"
 * * case pivotFace = "t_pf"
 * * case pivotAlign = "t_pa"
 * * case pivotOffset = "t_po"
 * * case center = "e_c"
 * * case boundsMin = "e_mi"
 * * case boundsMax = "e_ma"
 * * case name = "n"
 *
 * So there are 27 pivot options, each can be offset by t_po, and the e_c encapsulates that offset + the volume default
 * offset which is (128,128, 0)
 *
 * ------------------
 *
 * vmaxb is the binary format
 *
 * stats, to help editor jobs
 *
 * VolumeStats
 * * case count
 * * case scount
 * * case min  //morton32
 * * case max  //morton32
 * * case emin // extent in case the workarea is less than 256^3
 * * case emax
 * * case smin // s means selected
 * * case smax
 * * case extent
 *
 * voxel_p_morton = stats.min + offset_in_chunk_data + chunk_min_morton
 *
 * the row of voxels saved inside a chunk doesn't necessarily starts from 0 and ends to 32^3
 * if the voxel is in the middle, you need to use stats.min and read till stats.max
 * if you place a voxel at 0,0,0 and at 31,31,31 inside the chunk, whole stuff is saved
 *
 * the volume itself is split in 32^3 chunks and saved in snapshots that come with a unique identifier:
 *
 * Snapshot Identifier
 * * case cid = "c" chunk id (morton)
 * * case sid = "s" snapshot id (timeline)
 * * case type = "t" type uint8 see below
 *
 * Snapshot Type
 * * case undoRestore = 0
 * * redoRestore
 * * undo
 * * redo
 * * checkpoint
 * * selection
 *
 * SnapshotStorage
 * * case data = "ds" -> binary voxels
 * * case layerColorUsage = "lc" ->stats
 * * case deselectedLayerColorUsage = "dlc"
 * * case stats = "st" VolumeStats (broken down to the chunk) I aggregate them for whole object
 * * case identifier = "id"
 *
 * the binary data is not RLE encoded, was too slow, got better size/speed with the overall lzfse compression, but
 * the binary data is morton encoded (I use morton 256, for object (aggregated from chunk id +local chunk coord) and
 * morton 32 for the chunks). morton256_x/y/z LUTs match voxel/Morton.h (x<<0, y<<1, z<<2).
 *
 * SamplerCoord (runtime lookup, not stored): t, chunkId, localId. Cache is 257 entries
 * (256 + 1 for rendered meshes). chunkId = morton256(dim)[t >> chunkExtent.order],
 * localId = morton256(dim)[t & chunkExtent.coordMax]. Equatable ignores t.
 *
 * a voxel has:
 * * 1 byte extended layer info - there are only 8 materials used for now 0-7 and 8 selected versions for them 8-15,
 *   with option to add more in the future up to 128
 * * 3 bytes position (not saved, given by morton index in the sequence)
 * * 1 byte palette index 0 means deleted
 *
 * just keep in mind the position is inferred at reading time using the morton encoding, so in the binary data you
 * only get 2 bytes palette + extendedLayer in a row for the whole max 32^3 chunk volume
 *
 * then chunk size is configurable in the app, but I found 32^3 is a good compromise for parallel cpu threading
 * across most ios devices old/new so you can take it as constant, doubt will change it anytime soon
 *
 * The only use-case where it makes sense to change this is when saving flat 2d voxel volumes, there I waste chunks
 * atm since just the floor is filled in. This is an optimization on my todo list (low prio).
 *
 * File formats are versioned though, so should be upgradeable in the future.
 *
 * @ingroup Formats
 */
class VMaxFormat : public PaletteFormat {
private:
	// scene.json
	struct VMaxObject {
		core::String n;						   // name
		core::String pal;					   // palette - only for objects, not for groups
		core::String data;					   // contents - only for objects, not for groups
		core::String hist;					   // history - only for objects, not for groups
		core::UUID id;						   // uuid
		core::UUID pid;						   // parent id
		core::String t_al;					   // alignment
		core::String t_pa;					   // pivotAlign
		core::String t_pf;					   // pivotFace
		core::String t_po;					   // pivotOffset
		glm::vec3 t_p{0.0f};				   // position (VoxelMax Z-up)
		glm::vec4 t_r{0.0f, 0.0f, 0.0f, 0.0f}; // SceneKit axis-angle: xyz = axis, w = radians
		glm::vec3 t_s{1.0f};				   // scale
		glm::vec3 ind{0.0f};				   // index
		glm::vec3 e_c{0.0f};				   // extent center
		glm::vec3 e_ma{0.0f};				   // extent max
		glm::vec3 e_mi{0.0f};				   // extent min
		bool s = true;						   // selected
		bool h = false;						   // hidden
	};

	// 8 layers (mi '1'..'8' -> voxel byte 0..7). PBR is per
	// layer, not per palette color. vmaxpsb keys: mi, sic, mc, rc, medium{t,i,a}, sh, name.
	struct VmaxMaterial {
		core::String name;
		double transmission = 0.0;
		double absorption = 0.0;
		double ior = 0.0;
		double roughness = 0.0;
		double metalness = 0.0;
		double emission = 0.0;
		bool enableShadows = true;
		bool hasTransmission = false;
		bool hasAbsorption = false;
		bool hasIor = false;
		bool hasRoughness = false;
		bool hasMetalness = false;
		bool hasEmission = false;
	};

	struct VmaxLayerMaterials {
		VmaxMaterial layers[8];
		bool present[8]{false, false, false, false, false, false, false, false};
		int activeLayer = 0; // voxel byte 0-7 from ali
	};

	// scene.json camera
	// angles are in euler angles
	// The camera might follow the Scenekit orientation, so Y is screen facing here (roll angle -> depth)
	// The camera is on a stick: (Camera posX,posY) - zoom - (Origin)
	struct VMaxCamera {
		float da = 0.0f;				 // anglesDepth
		float ha = 0.19591334462165833f; // anglesHeight
		float wa = 0.0f;				 // anglesWidth
		float lda = 0.0f;				 // anglesLightsDepth
		float lha = 1.8209133148193359f; // anglesLightsHeight
		float lwa = 0.25f;				 // anglesLightsWidth
		glm::vec3 o{0.0f};				 // origin
		float px = 0.0f;				 // positionX
		float py = 0.0f;				 // positionY
		float z = 512.0f;				 // zoom
	};

	// scene.json
	struct VMaxGroup {
		core::UUID id;
		core::String name;
		core::UUID pid;					   // parent id
		glm::vec3 t_p{0.0f};				   // position (VoxelMax Z-up)
		glm::vec4 t_r{0.0f, 0.0f, 0.0f, 0.0f}; // SceneKit axis-angle: xyz = axis, w = radians
		glm::vec3 t_s{1.0f};				   // scale
		glm::vec3 e_c{0.0f};				   // extent center
		glm::vec3 e_ma{0.0f};				   // extent max
		glm::vec3 e_mi{0.0f};				   // extent min
		bool s = true;						   // selected
		bool h = false;						   // hidden
	};
	// scene nodes hierarchy, groups and volumes
	struct VMaxScene {
		// objects are instances of models
		core::DynamicArray<VMaxObject> objects;
		core::DynamicArray<VMaxGroup> groups;
		VMaxCamera cam;
		int v = 0;				 //
		bool nrn = true;		 //
		bool ssr = false;		 //
		float lint = 0.5f;		 //
		float aint = 0.25f;		 //
		float eint = 0.5f;		 //
		core::String af = "t";	 //
		float bloombrad = 4.0f;	 //
		float bloomint = 0.6f;	 //
		float bloomthr = 1.0f;	 //
		float outlineint = 0.4f; //
		float outlinesz = 2.0f;	 //
		float sat = 1.0f;		 //
		float shadowint = 1.0f;	 //
		float temp = 0.0f;		 //
		float tint = 0.0f;		 //
		float cont = 0.0f;		 //

		core::String background = "#FBFBFBFF"; //
		core::String lcolor = "#FFFFFFFF";	   //
	};

	void applySceneTransform(scenegraph::SceneGraphNode &node, const glm::vec3 &t_p, const glm::vec4 &t_r,
							 const glm::vec3 &t_s) const;
	void applySceneTransform(scenegraph::SceneGraphNode &node, const glm::vec3 &t_p, const glm::vec4 &t_r,
							 const glm::vec3 &t_s, const glm::vec3 &volumeMins) const;

	bool loadSceneJson(const io::ArchivePtr &archive, VMaxScene &scene, const core::String &sceneJsonPath) const;
	bool loadScenePackage(const io::ArchivePtr &archive, const core::String &packageDir,
						  scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx);
	bool loadObjectFromArchive(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx, const VMaxObject &obj,
							   const palette::Palette &palette, const VmaxLayerMaterials &layers) const;
	bool loadObject(const core::String &filename, io::SeekableReadStream *data, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx, const VMaxObject &obj, const palette::Palette &palette,
					const VmaxLayerMaterials &layers) const;
	bool loadPaletteFromArchive(const io::ArchivePtr &archive, const core::String &paletteName,
								palette::Palette &palette, const LoadContext &ctx, VmaxLayerMaterials &layers) const;
	bool loadPaletteFromVmaxb(io::SeekableReadStream &stream, palette::Palette &palette,
							  VmaxLayerMaterials &layers) const;
	void applyVmaxPaletteSettings(const util::BinaryPList &plist, palette::Palette &palette,
								  VmaxLayerMaterials &layers) const;
	void applyVmaxLayerMaterial(palette::Palette &palette, uint8_t palIdx, const VmaxMaterial &material) const;
	bool parseVmaxMaterial(const util::BinaryPList &node, VmaxMaterial &material, int &layer) const;
	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;

public:
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override {
		return false;
	}
	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	static const io::FormatDescription &format() {
		static io::FormatDescription f{
			"VoxelMax", "", {"vmax.zip", "vmax", "vmaxb"}, {}, VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED};
		return f;
	}
};

} // namespace voxelformat
