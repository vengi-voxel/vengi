set(LIB voxelformat)
set(SRCS
	external/ogt_vox.h
	external/tiny_obj_loader.h
	external/tiny_gltf.h
	external/json.hpp
	external/ufbx.h external/ufbx.c
	external/libvxl.h external/libvxl.c

	private/BinaryPList.h private/BinaryPList.cpp
	private/MinecraftPaletteMap.h private/MinecraftPaletteMap.cpp
	private/NamedBinaryTag.h private/NamedBinaryTag.cpp
	private/SchematicIntReader.h private/SchematicIntWriter.h
	private/Tri.h private/Tri.cpp

	Format.h Format.cpp
	FormatConfig.h FormatConfig.cpp
	FormatThumbnail.h
	AnimaToonFormat.h AnimaToonFormat.cpp
	AoSVXLFormat.h AoSVXLFormat.cpp
	BinVoxFormat.h BinVoxFormat.cpp
	CSMFormat.h CSMFormat.cpp
	DatFormat.h DatFormat.cpp
	FBXFormat.h FBXFormat.cpp
	GoxFormat.h GoxFormat.cpp
	KVXFormat.h KVXFormat.cpp
	KV6Format.h KV6Format.cpp
	QBTFormat.h QBTFormat.cpp
	QBFormat.h QBFormat.cpp
	QBCLFormat.h QBCLFormat.cpp
	QEFFormat.h QEFFormat.cpp
	MCRFormat.h MCRFormat.cpp
	MTSFormat.h MTSFormat.cpp
	VXAFormat.h VXAFormat.cpp
	VXCFormat.h VXCFormat.cpp
	VXMFormat.h VXMFormat.cpp
	VXRFormat.h VXRFormat.cpp
	VXTFormat.h VXTFormat.cpp
	VXLFormat.h VXLFormat.cpp
	VMaxFormat.h VMaxFormat.cpp
	MeshFormat.h MeshFormat.cpp
	CubFormat.h CubFormat.cpp
	OBJFormat.h OBJFormat.cpp
	PLYFormat.h PLYFormat.cpp
	QuakeBSPFormat.h QuakeBSPFormat.cpp
	SproxelFormat.h SproxelFormat.cpp
	SchematicFormat.h SchematicFormat.cpp
	SMFormat.h SMFormat.cpp
	STLFormat.h STLFormat.cpp
	VoxFormat.h VoxFormat.cpp
	SLAB6VoxFormat.h SLAB6VoxFormat.cpp
	VolumeFormat.h VolumeFormat.cpp
	GLTFFormat.h GLTFFormat.cpp
	VENGIFormat.h VENGIFormat.cpp
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} DEPENDENCIES scenegraph)

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
	tests/OBJFormatTest.cpp
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
	tests/minecraft_110.mca
	tests/minecraft_113.mca
	tests/magicavoxel.vox
	tests/cube.obj
	tests/cube.mtl
	tests/cube.stl
	tests/ascii.stl
	tests/test.gox
	tests/test.plist
	tests/test.vxm
	tests/test2.vxm
	tests/8ontop.vox
	tests/cw.cub
	tests/crabby.qbcl
	tests/crabby.vox
	tests/chr_knight.gox
	tests/r.0.-2.mca
	tests/r.0.-2.qb
	tests/rgb.vxl
	tests/rgb.csv
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
	tests/vox_character.vox
	tests/vox_glasses.vox
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
