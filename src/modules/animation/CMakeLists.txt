set(LIB animation)
set(SRCS
	chr/anim/Idle.cpp chr/anim/Idle.h
	chr/anim/Run.cpp chr/anim/Run.h
	chr/anim/Jump.cpp chr/anim/Jump.h
	chr/anim/Glide.cpp chr/anim/Glide.h
	chr/anim/Swim.cpp chr/anim/Swim.h
	chr/anim/Tool.cpp chr/anim/Tool.h
	chr/Character.cpp chr/Character.h
	chr/CharacterSkeleton.cpp chr/CharacterSkeleton.h
	chr/CharacterSkeletonAttribute.cpp chr/CharacterSkeletonAttribute.h

	animal/bird/anim/Idle.cpp animal/bird/anim/Idle.h
	animal/bird/anim/Run.cpp animal/bird/anim/Run.h
	animal/bird/Bird.cpp animal/bird/Bird.h
	animal/bird/BirdSkeleton.cpp animal/bird/BirdSkeleton.h
	animal/bird/BirdSkeletonAttribute.cpp animal/bird/BirdSkeletonAttribute.h

	Animation.cpp Animation.h
	AnimationCache.cpp AnimationCache.h
	AnimationSettings.cpp AnimationSettings.h
	AnimationRenderer.cpp AnimationRenderer.h
	AnimationEntity.cpp AnimationEntity.h
	Bone.cpp Bone.h
	BoneId.cpp BoneId.h
	BoneUtil.h
	Skeleton.h Skeleton.cpp
	SkeletonAttribute.h
)
set(SRCS_SHADERS
	shaders/skeleton.vert shaders/skeleton.frag
	shaders/skeletondepthmap.vert shaders/skeletondepthmap.frag
	shaders/skeletonshadowmap.vert shaders/skeletonshadowmap.frag
)
set(FILES
	voxel/models/characters/human/female/head/worker.qb
	voxel/models/characters/human/female/foot/worker.qb
	voxel/models/characters/human/female/hand/worker.qb
	voxel/models/characters/human/female/pants/worker.qb
	voxel/models/characters/human/female/belt/worker.qb
	voxel/models/characters/human/female/chest/worker.qb

	voxel/models/characters/dwarf/male/head/blacksmith.qb
	voxel/models/characters/dwarf/male/shoulder/blacksmith.qb
	voxel/models/characters/dwarf/male/foot/blacksmith.qb
	voxel/models/characters/dwarf/male/hand/blacksmith.qb
	voxel/models/characters/dwarf/male/pants/blacksmith.qb
	voxel/models/characters/dwarf/male/belt/blacksmith.qb
	voxel/models/characters/dwarf/male/chest/blacksmith.qb

	voxel/models/characters/human/male/head/worker.qb
	voxel/models/characters/human/male/foot/worker.qb
	voxel/models/characters/human/male/hand/worker.qb
	voxel/models/characters/human/male/pants/worker.qb
	voxel/models/characters/human/male/belt/worker.qb
	voxel/models/characters/human/male/chest/worker.qb

	voxel/models/characters/human/male/head/blacksmith.qb
	voxel/models/characters/human/male/shoulder/blacksmith.qb
	voxel/models/characters/human/male/foot/blacksmith.qb
	voxel/models/characters/human/male/hand/blacksmith.qb
	voxel/models/characters/human/male/pants/blacksmith.qb
	voxel/models/characters/human/male/belt/blacksmith.qb
	voxel/models/characters/human/male/chest/blacksmith.qb

	voxel/models/characters/human/male/head/knight.qb
	voxel/models/characters/human/male/shoulder/knight.qb
	voxel/models/characters/human/male/foot/knight.qb
	voxel/models/characters/human/male/hand/knight.qb
	voxel/models/characters/human/male/pants/knight.qb
	voxel/models/characters/human/male/belt/knight.qb
	voxel/models/characters/human/male/chest/knight.qb

	voxel/models/characters/human/male/head/shepherd.qb
	voxel/models/characters/human/male/shoulder/shepherd.qb
	voxel/models/characters/human/male/foot/shepherd.qb
	voxel/models/characters/human/male/hand/shepherd.qb
	voxel/models/characters/human/male/pants/shepherd.qb
	voxel/models/characters/human/male/belt/shepherd.qb
	voxel/models/characters/human/male/chest/shepherd.qb

	voxel/models/characters/undead/male/head/zombie.qb
	voxel/models/characters/undead/male/foot/zombie.qb
	voxel/models/characters/undead/male/hand/zombie.qb
	voxel/models/characters/undead/male/pants/zombie.qb
	voxel/models/characters/undead/male/belt/zombie.qb
	voxel/models/characters/undead/male/chest/zombie.qb

	voxel/models/characters/undead/male/shoulder/skeleton.qb
	voxel/models/characters/undead/male/head/skeleton.qb
	voxel/models/characters/undead/male/foot/skeleton.qb
	voxel/models/characters/undead/male/hand/skeleton.qb
	voxel/models/characters/undead/male/pants/skeleton.qb
	voxel/models/characters/undead/male/belt/skeleton.qb
	voxel/models/characters/undead/male/chest/skeleton.qb

	voxel/models/animals/chicken/head/head.qb
	voxel/models/animals/chicken/foot/foot.qb
	voxel/models/animals/chicken/body/body.qb
	voxel/models/animals/chicken/wing/wing.qb

	voxel/models/glider.qb
	voxel/models/items/axe-1.qb
	voxel/models/items/hammer-1.qb
	voxel/models/items/sword-1.qb
	voxel/models/items/sword-2.qb
)
set(LUA_SRCS
	chr/bones.lua
	chr/shared.lua

	chr/dwarf-male-blacksmith.lua

	chr/human-male-blacksmith.lua
	chr/human-male-knight.lua
	chr/human-male-shepherd.lua
	chr/human-male-worker.lua
	chr/human-female-worker.lua

	chr/undead-male-zombie.lua
	chr/undead-male-skeleton.lua

	animal/animal-chicken.lua
	animal/bird-shared.lua
	animal/bird-bones.lua
	animal/shared.lua
)
engine_add_module(TARGET ${LIB} SRCS ${SRCS} ${SRCS_SHADERS} FILES ${FILES} LUA_SRCS ${LUA_SRCS} DEPENDENCIES stock voxelformat render)
generate_shaders(${LIB} skeleton skeletonshadowmap skeletondepthmap)

set(TEST_SRCS
	tests/CharacterSettingsTest.cpp
	tests/SkeletonTest.cpp
)

gtest_suite_sources(tests ${TEST_SRCS})
gtest_suite_deps(tests ${LIB})

gtest_suite_begin(tests-${LIB} TEMPLATE ${ROOT_DIR}/src/modules/core/tests/main.cpp.in)
gtest_suite_sources(tests-${LIB} ${TEST_SRCS} ../core/tests/AbstractTest.cpp)
gtest_suite_deps(tests-${LIB} ${LIB})
gtest_suite_end(tests-${LIB})
