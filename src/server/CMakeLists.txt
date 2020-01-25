project(server)
set(SRCS
	Server.cpp Server.h
)
# These lua scripts are matching up with the entity type enum
set(LUA_BEHAVIOURTREES
	ai/animal-rabbit.lua
	ai/animal-wolf.lua
	ai/dwarf-male-blacksmith.lua
	ai/human-male-blacksmith.lua
	ai/human-male-knight.lua
	ai/human-male-shepherd.lua
	ai/human-male-worker.lua
	ai/undead-male-skeleton.lua
	ai/undead-male-zombie.lua
	ai/shared.lua
	behaviourtrees.lua
)
set(LUA_ATTRIBUTES
	attrib/animal-rabbit.lua
	attrib/animal-wolf.lua
	attrib/dwarf-male-blacksmith.lua
	attrib/human-male-blacksmith.lua
	attrib/human-male-knight.lua
	attrib/human-male-shepherd.lua
	attrib/human-male-worker.lua
	attrib/human-female-worker.lua
	attrib/undead-male-skeleton.lua
	attrib/undead-male-zombie.lua
	attrib/shared.lua
	attributes.lua
)
set(LUA_EVENTS
	event/generic.lua
	events.lua
)
set(LUA_MAPS
	map/map001.lua
)
set(LUA_SRCS ${LUA_BEHAVIOURTREES} ${LUA_ATTRIBUTES} ${LUA_EVENTS} ${LUA_MAPS})

set(FILES
	server/server-autoexec.cfg
)
set(MANIFESTS
	deployment/deployment.yaml.in
	deployment/postgres-config.yaml.in
	deployment/postgres-deployment.yaml.in
	deployment/postgres-service.yaml.in
	deployment/postgres-storage.yaml.in
	deployment/secrets.yaml.in
	deployment/service.yaml.in
)

engine_add_executable(TARGET ${PROJECT_NAME} SRCS ${SRCS} FILES ${FILES} LUA_SRCS ${LUA_SRCS})
engine_target_link_libraries(TARGET ${PROJECT_NAME} DEPENDENCIES backend http)
engine_docker(${PROJECT_NAME})
engine_manifests(${PROJECT_NAME} ${MANIFESTS})
