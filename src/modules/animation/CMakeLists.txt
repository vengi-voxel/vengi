set(LIB animation)
set(SRCS
	anim/Idle.cpp anim/Idle.h
	anim/Run.cpp anim/Run.h
	anim/Jump.cpp anim/Jump.h
	anim/Glide.cpp anim/Glide.h
	anim/Tool.cpp anim/Tool.h

	Animation.cpp Animation.h
	Bone.cpp Bone.h
	BoneId.h
	BoneUtil.h
	CharacterMeshType.cpp CharacterMeshType.h
	Character.cpp Character.h
	CharacterCache.cpp CharacterCache.h
	CharacterRenderer.cpp CharacterRenderer.h
	CharacterSettings.cpp CharacterSettings.h
	Skeleton.cpp Skeleton.h
	SkeletonAttribute.h
	LUAFunctions.h
)
set(SRCS_SHADERS
	shaders/character.vert shaders/character.frag
)
set(FILES
	characters/models/characters/human/male/heads/worker.vox
	characters/models/characters/human/male/feets/worker.vox
	characters/models/characters/human/male/hands/worker.vox
	characters/models/characters/human/male/pants/worker.vox
	characters/models/characters/human/male/belts/worker.vox
	characters/models/characters/human/male/chests/worker.vox

	characters/models/characters/human/male/heads/blacksmith.vox
	characters/models/characters/human/male/shoulders/blacksmith.vox
	characters/models/characters/human/male/feets/blacksmith.vox
	characters/models/characters/human/male/hands/blacksmith.vox
	characters/models/characters/human/male/pants/blacksmith.vox
	characters/models/characters/human/male/belts/blacksmith.vox
	characters/models/characters/human/male/chests/blacksmith.vox

	characters/models/characters/human/male/heads/knight.vox
	characters/models/characters/human/male/shoulders/knight.vox
	characters/models/characters/human/male/feets/knight.vox
	characters/models/characters/human/male/hands/knight.vox
	characters/models/characters/human/male/pants/knight.vox
	characters/models/characters/human/male/belts/knight.vox
	characters/models/characters/human/male/chests/knight.vox

	characters/models/characters/undead/male/heads/default.vox
	characters/models/characters/undead/male/feets/default.vox
	characters/models/characters/undead/male/hands/default.vox
	characters/models/characters/undead/male/pants/default.vox
	characters/models/characters/undead/male/belts/default.vox
	characters/models/characters/undead/male/chests/default.vox

	characters/models/glider.vox
	characters/models/items/axe-1.vox
	characters/models/items/hammer-1.vox
	characters/models/items/sword-1.vox
	characters/models/items/sword-2.vox
)
set(LUA_SRCS
	chr/undead-male-default.lua

	chr/human-male-worker.lua
	chr/human-male-blacksmith.lua
	chr/human-male-knight.lua
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
