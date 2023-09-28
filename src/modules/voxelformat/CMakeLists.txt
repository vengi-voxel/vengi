set(LIB voxelformat)
set(SRCS
	external/ogt_vox.h
	external/tiny_obj_loader.h
	external/tiny_gltf.h
	external/ufbx.h external/ufbx.c
	external/libvxl.h external/libvxl.c

	Format.h Format.cpp
	FormatConfig.h FormatConfig.cpp
	FormatThumbnail.h
	VolumeFormat.h VolumeFormat.cpp

	private/aceofspades/AoSVXLFormat.h       private/aceofspades/AoSVXLFormat.cpp
	private/animatoon/AnimaToonFormat.h      private/animatoon/AnimaToonFormat.cpp
	private/binvox/BinVoxFormat.h            private/binvox/BinVoxFormat.cpp
	private/chronovox/CSMFormat.h            private/chronovox/CSMFormat.cpp
	private/commandconquer/VXLFormat.h       private/commandconquer/VXLFormat.cpp
	private/cubeworld/CubFormat.h            private/cubeworld/CubFormat.cpp
	private/goxel/GoxFormat.h                private/goxel/GoxFormat.cpp
	private/magicavoxel/XRawFormat.h         private/magicavoxel/XRawFormat.cpp
	private/magicavoxel/VoxFormat.h          private/magicavoxel/VoxFormat.cpp
	private/magicavoxel/MagicaVoxel.h        private/magicavoxel/MagicaVoxel.cpp
	private/mesh/MeshFormat.h                private/mesh/MeshFormat.cpp
	private/mesh/OBJFormat.h                 private/mesh/OBJFormat.cpp
	private/mesh/PLYFormat.h                 private/mesh/PLYFormat.cpp
	private/mesh/STLFormat.h                 private/mesh/STLFormat.cpp
	private/mesh/Tri.h                       private/mesh/Tri.cpp
	private/mesh/quake/QuakeBSPFormat.h      private/mesh/quake/QuakeBSPFormat.cpp
	private/mesh/quake/MD2Format.h           private/mesh/quake/MD2Format.cpp
	private/mesh/FBXFormat.h                 private/mesh/FBXFormat.cpp
	private/mesh/GLTFFormat.h                private/mesh/GLTFFormat.cpp
	private/minecraft/DatFormat.h            private/minecraft/DatFormat.cpp
	private/minecraft/MCRFormat.h            private/minecraft/MCRFormat.cpp
	private/minecraft/MTSFormat.h            private/minecraft/MTSFormat.cpp
	private/minecraft/SchematicFormat.h      private/minecraft/SchematicFormat.cpp
	private/minecraft/MinecraftPaletteMap.h  private/minecraft/MinecraftPaletteMap.cpp
	private/minecraft/NamedBinaryTag.h       private/minecraft/NamedBinaryTag.cpp
	private/minecraft/SchematicIntReader.h   private/minecraft/SchematicIntWriter.h
	private/qubicle/QBTFormat.h              private/qubicle/QBTFormat.cpp
	private/qubicle/QBFormat.h               private/qubicle/QBFormat.cpp
	private/qubicle/QBCLFormat.h             private/qubicle/QBCLFormat.cpp
	private/qubicle/QEFFormat.h              private/qubicle/QEFFormat.cpp
	private/sandbox/VXAFormat.h              private/sandbox/VXAFormat.cpp
	private/sandbox/VXCFormat.h              private/sandbox/VXCFormat.cpp
	private/sandbox/VXMFormat.h              private/sandbox/VXMFormat.cpp
	private/sandbox/VXRFormat.h              private/sandbox/VXRFormat.cpp
	private/sandbox/VXTFormat.h              private/sandbox/VXTFormat.cpp
	private/slab6/KVXFormat.h                private/slab6/KVXFormat.cpp
	private/slab6/KV6Format.h                private/slab6/KV6Format.cpp
	private/slab6/SLAB6VoxFormat.h           private/slab6/SLAB6VoxFormat.cpp
	private/slab6/SLABShared.h
	private/sproxel/SproxelFormat.h          private/sproxel/SproxelFormat.cpp
	private/starmade/SMFormat.h              private/starmade/SMFormat.cpp
	private/vengi/VENGIFormat.h              private/vengi/VENGIFormat.cpp
	private/voxel3d/V3AFormat.h              private/voxel3d/V3AFormat.cpp
	private/voxelbuilder/VBXFormat.h         private/voxelbuilder/VBXFormat.cpp
	private/voxelmax/BinaryPList.h           private/voxelmax/BinaryPList.cpp
	private/voxelmax/VMaxFormat.h            private/voxelmax/VMaxFormat.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES scenegraph json)

set(TEST_SRCS
	tests/AbstractVoxFormatTest.h tests/AbstractVoxFormatTest.cpp
	tests/AnimaToonFormatTest.cpp
	tests/AoSVXLFormatTest.cpp
	tests/BinVoxFormatTest.cpp
	tests/ConvertTest.cpp
	tests/FormatPaletteTest.cpp
	tests/CSMFormatTest.cpp
	tests/CubFormatTest.cpp
	tests/GLTFFormatTest.cpp
	tests/GoxFormatTest.cpp
	tests/KVXFormatTest.cpp
	tests/KV6FormatTest.cpp
	tests/MeshFormatTest.cpp
	tests/MCRFormatTest.cpp
	tests/MD2FormatTest.cpp
	tests/OBJFormatTest.cpp
	tests/PLYFormatTest.cpp
	tests/QBTFormatTest.cpp
	tests/QBFormatTest.cpp
	tests/QBCLFormatTest.cpp
	tests/QEFFormatTest.cpp
	tests/QuakeBSPFormatTest.cpp
	tests/SchematicFormatTest.cpp
	tests/SproxelFormatTest.cpp
	tests/SLAB6VoxFormatTest.cpp
	tests/SMFormatTest.cpp
	tests/STLFormatTest.cpp
	tests/V3AFormatTest.cpp
	tests/VBXFormatTest.cpp
	tests/VolumeFormatTest.cpp
	tests/VoxFormatTest.cpp
	tests/VXLFormatTest.cpp
	tests/VXRFormatTest.cpp
	tests/VXMFormatTest.cpp
	tests/VENGIFormatTest.cpp
	tests/VMaxFormatTest.cpp

	tests/BinaryPListTest.cpp
	tests/MinecraftPaletteMapTest.cpp
	tests/NamedBinaryTagTest.cpp
	tests/TriTest.cpp

	tests/TestHelper.cpp tests/TestHelper.h
	tests/8ontop.h
	tests/vox_character.h
	tests/vox_glasses.h
)
set(TEST_FILES
	tests/palette-nippon.png

	tests/qubicle.qb
	tests/qubicle.qbt
	tests/qubicle.qef
	tests/qubicle.qbcl
	tests/testload.qef
	tests/aceofspades.vxl
	tests/chronovox-studio.csm
	tests/cc.vxl
	tests/cc.hva
	tests/hmec.vxl
	tests/hmec.hva
	tests/test.binvox
	tests/test.kvx
	tests/test.kv6
	tests/test2.kv6
	tests/minecraft_110.mca
	tests/minecraft_113.mca
	tests/magicavoxel.vox
	tests/cube.mtl
	tests/cube.obj
	tests/cube.ply
	tests/cube.stl
	tests/ascii.stl
	tests/ascii.ply
	tests/test.gox
	tests/test.plist
	tests/palette.settings.vmaxpsb
	tests/0voxel.vmax.zip
	tests/1voxel.vmax.zip
	tests/2voxel.vmax.zip
	tests/5voxel.vmax.zip
	tests/test.vxm
	tests/test2.vxm
	tests/8ontop.vox
	tests/cw.cub
	tests/crabby.qbcl
	tests/crabby.vox
	tests/chr_knight.gox
	tests/kvx_save.qb
	tests/r.0.-2.mca
	tests/r.0.-2.qb
	tests/rgb.vxl
	tests/rgb.csv
	tests/rgb.gltf
	tests/rgb.vox
	tests/rgb.gox
	tests/rgb.qb
	tests/rgb.qef
	tests/rgb.vxm
	tests/rgb.cub
	tests/rgb.qbcl
	tests/rgb_small.cub
	tests/rgb_small.qb
	tests/rgb_small.qbcl
	tests/rgb_small.qbt
	tests/rgb_small.vox
	tests/rgb_small.vxm
	tests/slab6_vox_test.kv6
	tests/slab6_vox_test.kvx
	tests/slab6_vox_test.vox
	tests/vox_character.vox
	tests/vox_glasses.vox
	tests/voxlap5.kv6
	tests/glTF/cube/Cube_BaseColor.png
	tests/glTF/cube/Cube_MetallicRoughness.png
	tests/glTF/cube/Cube.bin
	tests/glTF/cube/Cube.gltf
	tests/glTF/BoxAnimated.glb
	tests/glTF/lantern/Lantern_baseColor.png
	tests/glTF/lantern/Lantern_emissive.png
	tests/glTF/lantern/Lantern_normal.png
	tests/glTF/lantern/Lantern_roughnessMetallic.png
	tests/glTF/lantern/Lantern.bin
	tests/glTF/lantern/Lantern.gltf
	"tests/e2de1723/e2de1723.Idle.vxa"
	"tests/e2de1723/e2de1723.vxr"
	"tests/e2de1723/Content (1).vxm"
	"tests/e2de1723/Content.vxm"
	"tests/giant_dinosaur/Head_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/Hip_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/L_Arm_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/L_Foot_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/L_ForeArm_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/L_Hand_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/L_Leg_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/L_Thigh_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/Mouth_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/Neck_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/R_Arm_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.Attack 01.vxa"
	"tests/giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.Death 01.vxa"
	"tests/giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.Death Pose 01.vxa"
	"tests/giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.Eat 01.vxa"
	"tests/giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.Eat 02.vxa"
	"tests/giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.Get Hit 01.vxa"
	"tests/giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.Idle 01.vxa"
	"tests/giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.Run 01.vxa"
	"tests/giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.vxr"
	"tests/giant_dinosaur/Reptiles_Biped_Giant_Dinossaur_V2.Walk 01.vxa"
	"tests/giant_dinosaur/R_Foot_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/R_ForeArm_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/R_Hand_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/R_Leg_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/Tail1_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/Tail2_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/Tail3_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/Tail4_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/Torax1_Reptiles_Biped_Medium_Dinosaur.vxm"
	"tests/giant_dinosaur/Torax2_Reptiles_Biped_Medium_Dinosaur.vxm"
	tests/voxelbuilder.vbx
	voxedit/robo.vox
	voxedit/robo.qb
	voxedit/chr_knight.qb
	voxedit/chr_knight.qbcl
	voxedit/chr_knight.gox
)

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS})
gtest_suite_files(tests-${LIB} ${TEST_FILES})
gtest_suite_deps(tests-${LIB} ${LIB} test-app video)
gtest_suite_end(tests-${LIB})
