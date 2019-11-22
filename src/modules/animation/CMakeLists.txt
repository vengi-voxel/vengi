set(LIB animation)
set(SRCS
	chr/anim/Idle.cpp chr/anim/Idle.h
	chr/anim/Run.cpp chr/anim/Run.h
	chr/anim/Jump.cpp chr/anim/Jump.h
	chr/anim/Glide.cpp chr/anim/Glide.h
	chr/anim/Tool.cpp chr/anim/Tool.h
	chr/CharacterMeshType.cpp chr/CharacterMeshType.h
	chr/Character.cpp chr/Character.h
	chr/CharacterCache.cpp chr/CharacterCache.h
	chr/CharacterSettings.cpp chr/CharacterSettings.h
	chr/CharacterSkeleton.cpp chr/CharacterSkeleton.h
	chr/CharacterSkeletonAttribute.h
	chr/LUAFunctions.h

	Animation.cpp Animation.h
	AnimationCache.h
	AnimationRenderer.cpp AnimationRenderer.h
	AnimationEntity.h
	Bone.cpp Bone.h
	BoneId.cpp BoneId.h
	BoneUtil.h
	Skeleton.h Skeleton.cpp
	SkeletonAttribute.h
)
set(SRCS_SHADERS
	shaders/character.vert shaders/character.frag
)
set(FILES
	voxel/models/characters/dwarf/male/head/blacksmith.vox
	voxel/models/characters/dwarf/male/shoulder/blacksmith.vox
	voxel/models/characters/dwarf/male/foot/blacksmith.vox
	voxel/models/characters/dwarf/male/hand/blacksmith.vox
	voxel/models/characters/dwarf/male/pants/blacksmith.vox
	voxel/models/characters/dwarf/male/belt/blacksmith.vox
	voxel/models/characters/dwarf/male/chest/blacksmith.vox

	voxel/models/characters/human/male/head/worker.vox
	voxel/models/characters/human/male/foot/worker.vox
	voxel/models/characters/human/male/hand/worker.vox
	voxel/models/characters/human/male/pants/worker.vox
	voxel/models/characters/human/male/belt/worker.vox
	voxel/models/characters/human/male/chest/worker.vox

	voxel/models/characters/human/male/head/blacksmith.vox
	voxel/models/characters/human/male/shoulder/blacksmith.vox
	voxel/models/characters/human/male/foot/blacksmith.vox
	voxel/models/characters/human/male/hand/blacksmith.vox
	voxel/models/characters/human/male/pants/blacksmith.vox
	voxel/models/characters/human/male/belt/blacksmith.vox
	voxel/models/characters/human/male/chest/blacksmith.vox

	voxel/models/characters/human/male/head/knight.vox
	voxel/models/characters/human/male/shoulder/knight.vox
	voxel/models/characters/human/male/foot/knight.vox
	voxel/models/characters/human/male/hand/knight.vox
	voxel/models/characters/human/male/pants/knight.vox
	voxel/models/characters/human/male/belt/knight.vox
	voxel/models/characters/human/male/chest/knight.vox

	voxel/models/characters/human/male/head/shepherd.vox
	voxel/models/characters/human/male/shoulder/shepherd.vox
	voxel/models/characters/human/male/foot/shepherd.vox
	voxel/models/characters/human/male/hand/shepherd.vox
	voxel/models/characters/human/male/pants/shepherd.vox
	voxel/models/characters/human/male/belt/shepherd.vox
	voxel/models/characters/human/male/chest/shepherd.vox

	voxel/models/characters/undead/male/head/zombie.vox
	voxel/models/characters/undead/male/foot/zombie.vox
	voxel/models/characters/undead/male/hand/zombie.vox
	voxel/models/characters/undead/male/pants/zombie.vox
	voxel/models/characters/undead/male/belt/zombie.vox
	voxel/models/characters/undead/male/chest/zombie.vox

	voxel/models/characters/undead/male/shoulder/skeleton.vox
	voxel/models/characters/undead/male/head/skeleton.vox
	voxel/models/characters/undead/male/foot/skeleton.vox
	voxel/models/characters/undead/male/hand/skeleton.vox
	voxel/models/characters/undead/male/pants/skeleton.vox
	voxel/models/characters/undead/male/belt/skeleton.vox
	voxel/models/characters/undead/male/chest/skeleton.vox

	voxel/models/glider.vox
	voxel/models/items/axe-1.vox
	voxel/models/items/hammer-1.vox
	voxel/models/items/sword-1.vox
	voxel/models/items/sword-2.vox
)
set(LUA_SRCS
	chr/bones.lua

	chr/dwarf-male-blacksmith.lua

	chr/human-male-blacksmith.lua
	chr/human-male-knight.lua
	chr/human-male-shepherd.lua
	chr/human-male-worker.lua

	chr/undead-male-zombie.lua
	chr/undead-male-skeleton.lua
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} FILES ${FILES} LUA_SRCS ${LUA_SRCS} DEPENDENCIES core attrib stock voxelformat render commonlua)
generate_shaders(${LIB} character)

set(TEST_SRCS
	tests/CharacterSettingsTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
