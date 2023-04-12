/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/DynamicArray.h"

namespace io {
class ZipArchive;
}

namespace voxelformat {

/**
 * @brief VoxelMax (*.vmax, *.vmax.zip)
 *
 * up to 2040 materials
 * 256x256x256 working area
 * unlimited history
 *
 * ------------------
 *
 * scene.json gives the scene nodes hierarchy, groups and volumes.
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
 * * case min
 * * case max
 * * case emin // extent in case the workarea is less than 256^3
 * * case emax
 * * case smin // s means selected
 * * case smax
 * * case extent
 *
 * the volume itself is split in 32^3 chunks and saved in snapshots that come with a unique identifier:
 *
 * Snapshot Identifier
 * * case cid = "c" chunk id
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
 * morton 32 for the chunks)
 *
 * a voxel has:
 * * 1 byte extended layer info - there are only 8 materials used for now 0-7 and 8 selected versions for them 8-15,
 * with option to add more in the future up to 128
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
		core::String id;					   // uuid
		core::String pid;					   // parent id
		core::String t_al;					   // alignment
		core::String t_pa;					   // pivotAlign
		core::String t_pf;					   // pivotFace
		core::String t_po;					   // pivotOffset
		glm::vec3 t_p{0.0f};				   // position
		glm::vec4 t_r{0.0f, 0.0f, 0.0f, 0.0f}; // rotation
		glm::vec3 t_s{1.0f};				   // scale
		glm::vec3 ind{0.0f};				   // index
		glm::vec3 e_c{0.0f};				   // center
		glm::vec3 e_ma{0.0f};				   // boundsMax
		glm::vec3 e_mi{0.0f};				   // boundsMin
		bool s = true;						   // selected
		bool h = false;						   // hidden
	};

	// scene.json
	struct VMaxCamera {
		float da = 0.0f;				 //
		float ha = 0.19591334462165833f; //
		float lda = 0.0f;				 //
		float lha = 1.8209133148193359f; //
		float lwa = 0.25f;				 //
		glm::vec3 o{0.0f};				 // origin
		float px = 0.0f;				 //
		float py = 0.0f;				 //
		float wa = 0.0f;				 //
		float z = 512.0f;				 //
	};

	// scene.json
	// scene nodes hierarchy, groups and volumes
	struct VMaxScene {
		core::DynamicArray<VMaxObject> objects;
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

	bool loadSceneJson(io::ZipArchive &zipArchive, VMaxScene &scene) const;
	bool loadObject(const core::String &filename, io::ZipArchive &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx, const VMaxObject &obj) const;
	bool loadPalette(io::ZipArchive &archive, const core::String &paletteName, voxel::Palette &palette,
					 const LoadContext &ctx) const;
	bool loadVolume(const core::String &filename, io::ZipArchive &archive, const LoadContext &ctx,
					const VMaxObject &obj, voxel::RawVolume *v) const;

	bool loadGroupsPalette(const core::String &filename, io::SeekableReadStream &stream,
						   scenegraph::SceneGraph &sceneGraph, voxel::Palette &palette,
						   const LoadContext &ctx) override;

public:
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream, const SaveContext &ctx) override {
		return false;
	}
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream &stream,
								   const LoadContext &ctx) override;
	size_t loadPalette(const core::String &filename, io::SeekableReadStream &stream, voxel::Palette &palette,
					   const LoadContext &ctx) override;
};

} // namespace voxelformat
