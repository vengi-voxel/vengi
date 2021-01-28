project(server)
set(SRCS
	Server.cpp Server.h
)

set(ENTITIES
	animal-rabbit
	animal-wolf
	dwarf-male-blacksmith
	human-male-blacksmith
	human-male-knight
	human-male-shepherd
	human-male-worker
	human-female-worker
	undead-male-skeleton
	undead-male-zombie
)

# These lua scripts are matching up with the entity type enum
set(LUA_BEHAVIOURTREES
	ai/shared.lua
	behaviourtrees.lua
	behaviourtreenodes.lua
)
set(LUA_ATTRIBUTES
	attrib/shared.lua
	attributes.lua
)

foreach(ENT ${ENTITIES})
	list(APPEND LUA_BEHAVIOURTREES "ai/${ENT}.lua")
	list(APPEND LUA_ATTRIBUTES "attrib/${ENT}.lua")
endforeach()

set(LUA_EVENTS
	event/generic.lua
	events.lua
)
set(LUA_MAPS
	map/map001.lua
)
set(LUA_SRCS
	${LUA_BEHAVIOURTREES}
	${LUA_ATTRIBUTES}
	${LUA_EVENTS}
	${LUA_MAPS}
	shared/entities.lua
)

set(FILES
	server/server-autoexec.cfg
)
set(MANIFESTS
	deployment/postgres-config.yaml.in
	deployment/postgres-deployment.yaml.in
	deployment/postgres-service.yaml.in
	deployment/postgres-storage.yaml.in
	deployment/postgres-secrets.yaml.in
	deployment/server-deployment.yaml.in
	deployment/server-service.yaml.in
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} LUA_SRCS ${LUA_SRCS})
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES backend)
engine_docker(${PROJECT_NAME})
engine_manifests(${PROJECT_NAME} ${MANIFESTS})
