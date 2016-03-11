fips_begin_module(backend)
	fips_dir(spawn)
	fips_files(
		SpawnMgr.cpp SpawnMgr.h
	)

	fips_dir(loop)
	fips_files(
		ServerLoop.cpp ServerLoop.h
	)

	fips_dir(entity/ai)
	fips_files(
		AICharacter.cpp AICharacter.h
		AIRegistry.cpp AIRegistry.h
		AILoader.h
	)

	fips_dir(network)
	fips_files(
		UserConnectHandler.cpp UserConnectHandler.h
		UserDisconnectHandler.h
		AttackHandler.h
		ServerNetworkModule.h
		MoveHandler.h
		IUserProtocolHandler.h
	)

	fips_dir(entity/ai/condition)
	fips_files(
		IsCloseToSelection.h
		IsSelectionAlive.h
		IsOnCooldown.h
	)

	fips_dir(entity/ai/action)
	fips_files(
		Task.h
		GoHome.h
		Spawn.h
		Die.h
		SetPointOfInterest.h
		AttackOnSelection.h
		TriggerCooldown.cpp TriggerCooldown.h
		TriggerCooldownOnSelection.cpp TriggerCooldownOnSelection.h
	)

	fips_dir(entity/ai/filter)
	fips_files(
		SelectPrey.cpp SelectPrey.h
		SelectVisible.cpp SelectVisible.h
		SelectIncreasePartner.cpp SelectIncreasePartner.h
		SelectNpcsOfTypes.h
	)

	fips_dir(storage)
	fips_files(
		UserStore.cpp UserStore.h
		PQHandle.cpp PQHandle.h
		StoreCmd.cpp StoreCmd.h
	)

	fips_dir(poi)
	fips_files(
		PoiProvider.cpp PoiProvider.h
	)

	fips_dir(entity)
	fips_files(
		Npc.cpp Npc.h
		User.cpp User.h
		EntityId.h
		EntityStorage.cpp EntityStorage.h
		Entity.cpp Entity.h
	)
	find_package(SDL2 REQUIRED)
	include_directories(${SDL2_INCLUDE_DIRS} .)
	fips_deps(network io core util voxel attrib cooldown dbpost simpleai ${SDL2_LIBRARIES})
fips_end_module()

gtest_begin(backend)
	fips_dir(tests)
	fips_files(
		SpawnMgrTest.cpp
		PoiProviderTest.cpp
		StorageTest.cpp
	)
	fips_deps(network io core util voxel attrib cooldown dbpost simpleai backend)
gtest_end()
